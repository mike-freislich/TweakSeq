#ifndef MY_SEQUENCER
#define MY_SEQUENCER
#include <Arduino.h>
#include "note.h"
#include "glide.h"
#include "controls.h"
#include "memory.h"
#include "ImTimer.h"

#pragma region CONSTANTS / ENUMS

const uint16_t MAXTEMPO = 8000;
const float TEMPODIV = 80.0;

#if (GRAPHING)
uint16_t serialCounter = 0;
#endif

enum PlayModes : byte
{
  FORWARD = 1,
  REVERSE,
  PINGPONG,
  CHAOS,
  CHAOS_CURVES
};
PlayModes playMode = FORWARD;

// EXTERNAL CLOCK
uint32_t lastClockExt = 0;
enum ClockMode
{
  CLK_INTERNAL,
  CLK_EXTERNAL
};
ClockMode clockMode = ClockMode::CLK_INTERNAL;
bool extClockTriggered = false;

enum SequencerState
{
  MODE_SEQUENCER,
  MODE_BANKSELECT,
  MODE_PATTERNSELECT
};

#pragma endregion

SequencerState currentMode = MODE_SEQUENCER;
ImTimer bpmClock;
ImTimer gateTimer;
ImTimer clockLedTimer;

class Sequencer
{
private:
  int bpmMilliseconds;
  ImTimer *_bpmClock = nullptr;
  ImTimer *_gateTimer = nullptr;
  ImTimer *_clockLedTimer = nullptr;
  short currentStep = -1;
  Note currentNote;
  Note previousNote;
  Glide glide;
  bool isPaused = true;
  short transpose = 0;

  uint16_t bpm = 120;
  uint8_t curveIndex = CURVE_B;
  float portamento = 0.2;

  uint8_t patternLength = 16;
  short direction = 1;

  uint8_t gateLength = 10;
  uint8_t gateOpen = 0;
  uint8_t octave = 1;

public:
  Sequencer(ImTimer *bpm, ImTimer *gate, ImTimer *clockLed)
  {
    _bpmClock = bpm;
    _gateTimer = gate;
    _clockLedTimer = clockLed;
    setClockMode(clockMode);
  }

  void setRecording(bool recordingOn)
  {
    if (recordingOn)
    {
      ShiftRegisterPWM::singleton->set(ledPLAY, ledFLASH);
      ShiftRegisterPWM::singleton->set(ledSHIFT, ledOFF);
      currentStep = 0;
      isPaused = true;
      //clearSequenceLights();
      //setSequencerStep(currentStep); //TODO: setSequencerStep()
    }
    else
    {
      ShiftRegisterPWM::singleton->set(ledPLAY, ledOFF);      
      this->pause();
    }
  }

  short getCurrentStep() { return currentStep; }

  int getTempo() { return bpmMilliseconds; }

  void setBpmMilliseconds(uint16_t bpm)
  {
    bpmMilliseconds = 60.0 / bpm * 1000;
    bpmClock.changeDuration(bpmMilliseconds);
  }

  void setTempo(uint32_t milliseconds) {
    bpmMilliseconds = milliseconds;
    bpmClock.changeDuration(milliseconds);
  }

  int getBpm() { return bpm; }
  void setGateLength(int value) { gateLength = value; }
  void setGlideTime(float value) { portamento = value; }
  void setPatternLength(int value) { patternLength = value; }
  void setCurveShape(int value) { glide.setCurve(static_cast<CurveType>(value)); }
  void setOctave(int value) { octave = value; }
  short getTranspose() { return transpose; }

  void setTranspose(short direction)
  {
    if (direction != 0)
      transpose = constrain(this->transpose + direction, -24, 24);
  }

  /* ---------------- CLOCK HANDLING  ----------------
    */
  void bpmClockTick()
  {    
    uint32_t elapsed = millis() - lastClockExt;
    if (elapsed > 2000)
      clockMode = ClockMode::CLK_INTERNAL;
   
    internalClockTrigger();
  }

  void setClockMode(ClockMode mode)
  {
    clockMode = mode;
  }

  void externalClockTrigger()
  {
    clockMode = ClockMode::CLK_EXTERNAL;

    unsigned long now = millis();
    unsigned long elapsed = now - lastClockExt;

    if (elapsed > 20)
    {
      lastClockExt = now;
      beatFrom(CLK_EXTERNAL);
      setTempo(elapsed);
    }

    if (elapsed > 2000) {
      clockMode = ClockMode::CLK_INTERNAL;
    }
  }

  void internalClockTrigger()
  {
    beatFrom(CLK_INTERNAL);
  }

  void beatFrom(ClockMode fromClock)
  {
    if (fromClock == clockMode)
    {
      _clockLedTimer->start(once, 20);
      ShiftRegisterPWM::singleton->set(ledClock, ledON);
      ShiftRegisterPWM::singleton->set(outClock, ledON);                
      beat();
    }
  }

  void clockLedOff()
  {
    ShiftRegisterPWM::singleton->set(ledClock,ledOFF);    
    ShiftRegisterPWM::singleton->set(outClock, ledOFF);    
  }

  void openGate()
  {
    gateTimer.start(once, gateLength / 100.0 * getTempo());
    gateOpen = 1;    
    ShiftRegisterPWM::singleton->set(outGate, ledON);
    ShiftRegisterPWM::singleton->set(ledGate, ledON);    
  }

  void closeGate()
  {
    gateOpen = 0;
    ShiftRegisterPWM::singleton->set(outGate, ledOFF);
    ShiftRegisterPWM::singleton->set(ledGate, ledOFF);    
  }

  void pause()
  {
    isPaused = true;
    closeGate();
    ShiftRegisterPWM::singleton->set(ledPLAY, ledOFF);    
  }

  void play()
  {
    isPaused = false;
    ShiftRegisterPWM::singleton->set(ledPLAY, ledON);    
  }

  bool isStepEditing() {return ShiftRegisterPWM::singleton->get(ledPLAY) == ledFLASH; }

  short selectStep(short knobDirection)
  {
    if (knobDirection != 0)
    {
      int x = currentStep;
      if (knobDirection > 0)
        currentStep = ((x + 1) < patternLength) ? (x + 1) : 0;
      if (knobDirection < 0)
        currentStep = (x > 0) ? (x - 1) : patternLength - 1;
      playNote();
    }
    return currentStep;
  }

  void stepForward()
  {
    currentStep = nextStep(currentStep);    
    //setSequencerStep(currentStep); // TODO: setSequencerStep()
  }

  void beat()
  {
    if (!isPaused)
    {
      currentStep = nextStep(currentStep);
      playNote();
      //setSequencerStep(currentStep); TODO: setSequencerStep()
    }
  }

  uint8_t nextStep(int x)
  {
    if (patternLength <= 1)
      return 0;

    uint8_t retVal;
    switch (playMode)
    {
    case FORWARD:
    {
      retVal = ((x + 1) < patternLength) ? (x + 1) : 0;
      break;
    }
    case REVERSE:
    {
      retVal = (x > 0) ? (x - 1) : patternLength - 1;
      break;
    }
    case CHAOS:
    case CHAOS_CURVES:
    {
      retVal = random(0, patternLength);
      break;
    }
    case PINGPONG:
    {
      short test = x + direction;
      if (test > patternLength - 1)
        direction = -1;
      if (test < 0)
        direction = 1;

      retVal = x + direction;
      break;
    }
    }
    return retVal;
  }

  uint16_t pitchToVoltage(uint16_t oct, uint16_t note)
  {
    // oct = 1 to 8
    // note = 1 to 12  C,C#,D,D#,E,F,F#,G,G#,A,A#,B
    uint8_t vInc = 40; // 0.040V
    uint16_t voltage = constrain(vInc * (12 * (oct - 1) + 1) + (note - 1) * vInc, 0, 3840);
    return voltage;
  }

  void changeCurve()
  {
    curveIndex = (curveIndex + 1) % 3;
    glide.setCurve(static_cast<CurveType>(curveIndex));
  }

  Note getKeyboardNote(uint8_t keyPressed)
  {
    Note note;
    note.stepNumber = currentStep;
    uint8_t stepData = ((octave - 1) * 12) + keyPressed;
    note.midiNote = stepData + MIDI_OFFSET;
    note.isRest = (stepData == REST);
    note.isTie = (stepData == TIE);
    note.octave = octave;
    note.pitch = keyPressed;
    note.voltage = pitchToVoltage(note.octave, note.pitch); //midiTable[stepData];
    return note;
  }

  Note getPatternNote(int atIndex)
  {
    Note note;
    note.stepNumber = atIndex;
    uint8_t stepData = pattern[atIndex];

    note.isRest = (stepData == REST);
    note.isTie = (stepData == TIE);

    if (!note.isRest && !note.isTie)
    {
      note.octave = (stepData / 12.0) + 1;
      note.pitch = (stepData % 12) + 1;
      note.midiNote = stepData + MIDI_OFFSET;
    }

    if (note.isRest)
    {
      note.voltage = 0;
    }
    else if (note.isTie)
    {
      note.pitch = currentNote.pitch;
      note.voltage = currentNote.voltage;
    }
    else
    {
      note.voltage = pitchToVoltage(note.octave, note.pitch);
    }
    currentNote = note;

    return note;
  }

  void setPatternNote(Note note)
  {

    uint8_t noteData;

    if (note.isRest)
    {
      noteData = REST;
    }
    else if (note.isTie)
    {
      noteData = TIE;
    }
    else
      noteData = note.pitch + (note.octave - 1) * 12;

    pattern[currentStep] = noteData;
    Note newNote = getPatternNote(currentStep);
    printData(newNote);
  }

  // send MIDI message
  void MIDImessage(int command, int MIDI_note, int MIDIvelocity)
  {
    #if (LOGGING)
    Serial.write(command);      //send note on or note off command
    Serial.write(MIDI_note);    //send pitch data
    Serial.write(MIDIvelocity); //send velocity data
    #endif
  }

  void playNote()
  {
    previousNote = currentNote;
    currentNote = getPatternNote(currentStep);
    if (playMode == CHAOS_CURVES)
      setCurveShape(random(4));
    if (currentNote.isRest)
    {
      currentNote.pitch = previousNote.pitch;
      currentNote.octave = previousNote.octave;
      currentNote.voltage = previousNote.voltage;
      currentNote.midiNote = previousNote.midiNote;
    }
    else
    {
      openGate();
      glide.begin(this->getTempo(), portamento, previousNote.voltage, currentNote.voltage);
    }

    printData(currentNote);

    /* MIDImessage(100, note.midiNote, 120);       // TODO: MIDI
      */
  }

  void pianoKeyPressed(uint8_t keyPressed)
  {
    previousNote = currentNote;
    currentNote = getKeyboardNote(keyPressed);

    openGate();
    glide.begin(this->getTempo(), portamento, previousNote.voltage, currentNote.voltage);

    if (isStepEditing())
    {
      setPatternNote(currentNote);
      stepForward();
    }
  }

  void patternInsertRest()
  {
    Note note;
    note.isRest = true;
    note.octave = 0;
    note.pitch = 0;
    setPatternNote(note);
    stepForward();
  }

  int16_t getPitchCV()
  {
#if (GRAPHING)
    serialCounter++;
    serialCounter %= 100;
    if (serialCounter == 0)
    {
      int GRAPHSCALE = 8;

      char buffer[100];
      sprintf(buffer, "oct:%d voltage:%d\t gate:%d encBtn:%d funcBtn:%d black:%d white:%d",
              octave * 1000,
              (int)(voltage * 2.083),
              gateOpen,
              analogRead(BUTTONS_ENCODER) * GRAPHSCALE,
              analogRead(BUTTONS_FUNC) * GRAPHSCALE,
              analogRead(BUTTONS_KBD_BLACK) * GRAPHSCALE,
              analogRead(BUTTONS_KBD_WHITE) * GRAPHSCALE);
      Serial.println(buffer);
    }
#endif
    return constrain(glide.getPitch() + transpose * 40, 0, 3850);
  }

  void printData(Note &note)
  {
    /*
      char buffer[100];
      sprintf(buffer, "[BPM: %04d ] - step: %02d, octave: %u, note: %u, voltage: %04u, \trest:%d, tie:%d",
              bpm, currentStep, note.octave, note.pitch, note.voltage, note.isRest, note.isTie);
      Serial.println(buffer);
      */
  }
};

#endif