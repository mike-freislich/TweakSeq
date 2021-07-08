#ifndef MY_SEQUENCER
#define MY_SEQUENCER
#include <Arduino.h>
#include "note.h"
#include "glide.h"
#include "controls.h"
#include "memory.h"
#include "dac.h"

const int MAXTEMPO=4000;

// SEQUENCER
MP4822 dac;

enum PlayModes: byte { FORWARD = 1, REVERSE = 2, PINGPONG = 3, CHAOS = 4 }; PlayModes playMode = FORWARD;

// EXTERNAL CLOCK
unsigned long lastClockExt = 0;
enum ClockMode { CLK_INTERNAL, CLK_EXTERNAL}; ClockMode clockMode = CLK_INTERNAL;
bool extClockTriggered = false;

enum SequencerState { MODE_SEQUENCER, MODE_BANKSELECT, MODE_PATTERNSELECT };
bool currentMode = MODE_SEQUENCER;


ImTimer bpmClock;
ImTimer gateTimer;
ImTimer clockLedTimer;


class Sequencer {

  private:
    int bpmMilliseconds;
    ImTimer* _bpmClock = nullptr;
    ImTimer* _gateTimer = nullptr;
    ImTimer* _clockLedTimer = nullptr;
    int currentStep = -1;
    Note currentNote;
    Note previousNote;
    Glide glide;
    bool isPaused = true;
    byte transpose = 0;
        
    int bpm = 120;
    int curveIndex = CURVE_B;
    float portamento = 0.2;

    int patternLength = 16;
    int direction = 1;

    int gateLength = 10;
    int gateOpen = 0;
    byte octave = 1;

    byte serialCounter = 0;

  public:
    Sequencer(ImTimer* bpm, ImTimer* gate, ImTimer* clockLed) {
      _bpmClock = bpm;
      _gateTimer = gate;
      _clockLedTimer = clockLed;
      setClockMode(clockMode);
    }

    void setRecording(bool recordingOn) {
      if (recordingOn) {               
        setLedState(ledPLAY, ledFLASH);   // record mode ON         
        setLedState(ledSHIFT, ledOFF);
        currentStep = 0;
        isPaused = true;
        setSequencerStep(currentStep); 
        log("recording started\n"); 
      } else {
        setLedState(ledPLAY, ledOFF);     // record mode OFF
        this->pause();
        log("recording stopped\n"); 
      }
    }
    int getCurrentStep() { return currentStep; }

    int getTempo() { return bpmMilliseconds; }

    void setBpmMilliseconds(int bpm) {
      bpmMilliseconds = 60.0 / bpm * 1000;
      bpmClock.changeDuration(bpmMilliseconds);
    }

    int getBpm() { return bpm; }
    void setGateLength(int value) { gateLength = value; }
    void setGlideTime(float value) {  portamento = value; }
    void setPatternLength(int value) { patternLength = value; }
    void setCurveShape(int value) { glide.setCurve(static_cast<CurveType>(value)); }
    void setOctave(int value) { octave = value; }

    /* ---------------- CLOCK HANDLING  ----------------
    */
    void bpmClockTick() {
      internalClockTrigger();
    }

    void setClockMode(ClockMode mode) {
      clockMode = mode;
    }

    void externalClockTrigger() {
      unsigned long now = millis();
      unsigned long elapsed = now - lastClockExt;
      if (elapsed > 30) {
        lastClockExt = now;
        beatFrom(CLK_EXTERNAL);
      }
    }

    void internalClockTrigger() {
      beatFrom(CLK_INTERNAL);
    }

    void beatFrom(ClockMode fromClock) {
      if (fromClock == clockMode) {
        _clockLedTimer->start(once, 20);
        ioSet(ledClock, true);
        sendClockSignal(true);
        beat();
      }
    }

    void clockLedOff() {
      ioSet(ledClock, false);
      sendClockSignal(false);
    }

    void openGate() {
      gateTimer.start(once, gateLength / 100.0 * getTempo());
      gateOpen = 5000;
      ioSet(outGate, true);
      setLedState(ledGate, ledON);
    }

    void closeGate() {
      gateOpen = 0;
      ioSet(outGate, false);
      setLedState(ledGate, ledOFF);
    }

    void pause() {
      isPaused = true;
      closeGate();
      setLedState(ledPLAY, ledOFF);
    }

    void play() { isPaused = false; ioSet(ledPLAY, true); }

    
    
    bool isStepEditing() {
      return getLedState(ledPLAY) == ledFLASH;
    }

    int selectStep(int knobDirection) {
      if (knobDirection != 0) {
        int x = currentStep;
        if (knobDirection > 0) currentStep = ((x + 1) < patternLength) ? (x + 1) : 0;
        if (knobDirection < 0) currentStep = (x > 0) ? (x - 1) : patternLength - 1;
        playNote();
      }
      return currentStep;
    }

    void stepForward() {
      currentStep = nextStep(currentStep);
      setSequencerStep(currentStep);
    }

    void beat() {

      if (!isPaused) {
        currentStep = nextStep(currentStep);
        playNote();
        setSequencerStep(currentStep);
      }
    }

    byte nextStep(int x) {
      if (patternLength <= 1) return 0;

      byte retVal;
      switch (playMode) {
        case FORWARD: {
            retVal = ((x + 1) < patternLength) ? (x + 1) : 0;
            break;
          }
        case REVERSE: {
            retVal = (x > 0) ? (x - 1) : patternLength - 1;
            break;
          }
        case CHAOS: {
            retVal = random(0, patternLength);
            break;
          }
        case PINGPONG: {
            int test = x + direction;
            if (test > patternLength - 1) direction = -1;
            if (test < 0) direction = 1;

            retVal = x + direction;
            break;
          }
      }
      return retVal;
    }

    uint16_t  pitchToVoltage(uint16_t oct, uint16_t note) {
      // oct = 1 to 8
      // note = 1 to 12  C,C#,D,D#,E,F,F#,G,G#,A,A#,B
      static int vInc = 40; // 0.040V
      uint16_t voltage = constrain(vInc * (12 * (oct - 1) + 1) + (note - 1) * vInc, 0, 3840);
      return voltage;
    }

    void changeCurve() {
      curveIndex = (curveIndex + 1) % 3;
      glide.setCurve(static_cast<CurveType>(curveIndex));
    }

    Note getKeyboardNote(byte keyPressed) {
      Note note;
      note.stepNumber = currentStep;
      byte stepData = ((octave - 1) * 12) + keyPressed;
      note.midiNote = stepData + MIDI_OFFSET;
      note.isRest = (stepData == REST);
      note.isTie = (stepData == TIE);
      note.octave = octave;
      note.pitch = keyPressed;
      note.voltage = pitchToVoltage(note.octave, note.pitch); //midiTable[stepData];
      return note;
    }

    Note getPatternNote(int atIndex) {
      Note note;
      note.stepNumber = atIndex;
      byte stepData = pattern[atIndex];

      note.isRest = (stepData == REST);
      note.isTie = (stepData == TIE);

      if (!note.isRest && !note.isTie) {
        note.octave = (stepData / 12.0) + 1;
        note.pitch = (stepData % 12) + 1;
        note.midiNote = stepData + MIDI_OFFSET;
      }

      if (note.isRest) {
        note.voltage = 0;
      } else if (note.isTie) {
        note.pitch = currentNote.pitch;
        note.voltage = currentNote.voltage;
      } else {
        note.voltage = pitchToVoltage(note.octave, note.pitch);
      }
      currentNote = note;

      return note;
    }

    void setPatternNote(Note note) {

      byte noteData;

      if (note.isRest) {
        noteData = REST;
      }
      else if (note.isTie) {
        noteData = TIE;
      }
      else
        noteData = note.pitch + (note.octave - 1) * 12;

      pattern[currentStep] = noteData;
      Note newNote = getPatternNote(currentStep);
      printData(newNote);
    }

    // send MIDI message
    void MIDImessage(int command, int MIDI_note, int MIDIvelocity) {
      Serial.write(command);      //send note on or note off command
      Serial.write(MIDI_note);    //send pitch data
      Serial.write(MIDIvelocity); //send velocity data
    }

    void playNote() {
      previousNote = currentNote;
      currentNote = getPatternNote(currentStep);

      if (currentNote.isRest) {
        currentNote.pitch = previousNote.pitch;
        currentNote.octave = previousNote.octave;
        currentNote.voltage = previousNote.voltage;
        currentNote.midiNote = previousNote.midiNote;
      } else {
        openGate();
        glide.begin(this->getTempo(), portamento, previousNote.voltage, currentNote.voltage);
        cvOut(0, glide.getPitch());
      }

      printData(currentNote);

      /* MIDImessage(100, note.midiNote, 120);       // TODO: MIDI
      */

    }

    void pianoKeyPressed(byte keyPressed) {
      previousNote = currentNote;
      currentNote = getKeyboardNote(keyPressed);

      openGate();
      glide.begin(this->getTempo(), portamento, previousNote.voltage, currentNote.voltage);
      cvOut(0, glide.getPitch());

      if (isStepEditing()) {
        setPatternNote(currentNote);
        stepForward();
      }
    }

    void patternInsertRest() {
      Note note;
      note.isRest = true;
      note.octave = 0;
      note.pitch = 0;
      setPatternNote(note);
      stepForward();
    }

    void cvOut(byte channel, int v) {
      String errmsg = "";
      dac.DAC_set(v, channel, 0, DAC_CS, errmsg);
    }

    void updateDAC() {
      int voltage = glide.getPitch();
      cvOut(0, voltage);

#if (GRAPHING)
      serialCounter ++;
      serialCounter %= 8;
      if (serialCounter == 0) {

        int GRAPHSCALE = 8;
        Serial.print("octave:");
        Serial.print(octave * 1000);
        Serial.print(",voltage-A:");
        Serial.print(voltage * 2.083);
        Serial.print(",gate:");
        Serial.print(gateOpen);
        Serial.print(",encoderBtn:");
        Serial.print(analogRead(BUTTONS_ENCODER) * GRAPHSCALE);
        Serial.print(",funcBtn:");
        Serial.print(analogRead(BUTTONS_FUNC) * GRAPHSCALE);
        Serial.print(",black:");
        Serial.print(analogRead(BUTTONS_KBD_BLACK) * GRAPHSCALE);
        Serial.print(",white:");
        Serial.print(analogRead(BUTTONS_KBD_WHITE) * GRAPHSCALE);
        Serial.println();
      }
#endif

    }


    void printData(Note &note) {
#if (LOGGING)
      char buffer[100];
      sprintf(buffer, "[BPM: %04d ] - step: %02d, octave: %u, note: %u, voltage: %04u, \trest:%d, tie:%d",
              bpm, currentStep, note.octave, note.pitch, note.voltage, note.isRest, note.isTie);
      Serial.println(buffer);
#endif
    }
};

#endif