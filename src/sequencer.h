#ifndef MY_SEQUENCER
#define MY_SEQUENCER

#include <Arduino.h>
#include "note.h"
#include "glide.h"
#include "controls.h"
#include "memory.h"
#include "ShiftRegisterPWM.h"
#include "SimpleTimer.h"
#include "dialog.h"
#include "uistate.h"

#pragma region CONSTANTS / ENUMS

const uint16_t MAXTEMPO = 8000;
const float TEMPODIV = 80.0;

enum PlayModes : uint8_t
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

#pragma endregion

class Sequencer
{
private:
  Dialog dialog = Dialog();
  short currentStep = -1;
  Note currentNote;
  Note previousNote;
  Glide glide;
  bool isPaused = true;
  short transpose = 0;

  uint16_t bpm = 120;
  uint8_t curveIndex = Glide::CurveType::CURVE_B;
  float portamento = 0.2;

  uint8_t patternLength = 16;
  short direction = 1;

  uint8_t gateLength = 10;
  uint8_t gateOpen = 0;
  uint8_t octave = 1;

  ShiftRegisterPWM *sreg;

  uint16_t  getBpmInMilliseconds() { return 60.0 / bpm * 1000; }
  void      setBpmInMilliseconds(uint32_t milliseconds) { bpmClock.timeout = milliseconds; }
  

public:
  SimpleTimer bpmClock = SimpleTimer();
  SimpleTimer gateTimer = SimpleTimer();
  SimpleTimer clockLedTimer = SimpleTimer();
  SimpleTimer dialogTimer = SimpleTimer();

  Sequencer()
  {
    clockMode = CLK_INTERNAL;
    sreg = ShiftRegisterPWM::singleton;       
  }

  void setRecording(bool startRecording)
  {
    if (startRecording)
    {
      sreg->set(ledPLAY, ledFLASH);
      sreg->set(ledSHIFT, ledOFF);
      currentStep = 0;
      isPaused = true;

      sreg->clearSequenceLights();
      sreg->set(currentStep % 16, LedState::ledON);
    }
    else
    {
      ShiftRegisterPWM::singleton->set(ledPLAY, ledOFF);
      this->pause();
    }
  }

  /**
   * Converts beats/minute to milliseconds in order to update
   * the Sequencer's BPM Clock timer
   * @param bpm specifies the beats per minute to set
   */
  void setBpm(uint16_t bpm) { bpmClock.timeout = 60.0 / bpm * 1000; }
  uint16_t getBpm() { return bpm; }
  void setGateLength(int value) { gateLength = value; }
  void setGlideTime(float value) { portamento = value; }
  void setPatternLength(int value) { patternLength = value; }
  void setCurveShape(Glide::CurveType value) { glide.setCurve(value); }
  void setOctave(uint8_t value) { octave = value; }
  int8_t getTranspose() { return transpose; }

  void setTranspose(int8_t direction)
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

  void externalClockTrigger()
  {
    clockMode = ClockMode::CLK_EXTERNAL;

    unsigned long now = millis();
    unsigned long elapsed = now - lastClockExt;

    if (elapsed > 20)
    {
      lastClockExt = now;
      beatFrom(CLK_EXTERNAL);
      setBpmInMilliseconds(elapsed);
    }

    if (elapsed > 2000)
      clockMode = ClockMode::CLK_INTERNAL;
  }

  void internalClockTrigger()
  {
    beatFrom(CLK_INTERNAL);
  }

  void beatFrom(ClockMode fromClock)
  {
    if (fromClock == clockMode)
    {
      clockLedTimer.start(20);
      sreg->set(ledClock, ledON);
      sreg->set(outClock, ledON);
      beat();
    }
  }

  void clockLedOff()
  {
    sreg->set(ledClock, ledOFF);
    sreg->set(outClock, ledOFF);
  }

  void openGate()
  {
    gateTimer.start(gateLength / 100.0 * getBpmInMilliseconds());
    gateOpen = 1;
    sreg->set(outGate, ledON);
    sreg->set(ledGate, ledON);
  }

  void closeGate()
  {
    gateOpen = 0;
    sreg->set(outGate, ledOFF);
    sreg->set(ledGate, ledOFF);
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

  bool isStepEditing() { return ShiftRegisterPWM::singleton->get(ledPLAY) == ledFLASH; }

  void setStep(byte step)
  {
    currentStep = step;
  }

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

  void setValuePicker(int16_t value, int16_t low, int16_t high, bool timed = true, uint16_t ms = DIALOG_TIMEOUT)
  {
    dialog.setDisplayValue(value, low, high, timed, ms);    
    dialog.writeoutDisplayBuffer(&ioData, &ioFlashData);
    dialog.show();
  }

  void displayStep()
  {
    if (!dialog.isVisible())
    {
      sreg->clearSequenceLights();
      sreg->set(currentStep % 8, LedState::ledON);
    }
  }

  void beat()
  {
    if (!isPaused)
    {
      currentStep = nextStep(currentStep);
      playNote();
      displayStep();
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

  int16_t getPitchCV() { return constrain(glide.getPitch() + transpose * 40, 0, 3850); }

  uint16_t pitchToVoltage(uint16_t oct, uint16_t note)
  {    
    uint8_t vInc = 40;
    uint16_t voltage = constrain(vInc * (12 * (oct - 1) + 1) + (note - 1) * vInc, 0, 3840);
    return voltage;
  }

  void changeCurve()
  {
    curveIndex = (curveIndex + 1) % 3;
    glide.setCurve((Glide::CurveType)curveIndex);
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
    note.voltage = pitchToVoltage(note.octave, note.pitch);
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
      setCurveShape((Glide::CurveType)random(4));
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
      glide.begin(this->getBpmInMilliseconds(), portamento, previousNote.voltage, currentNote.voltage);
    }

    /* MIDImessage(100, note.midiNote, 120);       // TODO: MIDI
      */
  }

  void pianoKeyPressed(uint8_t keyPressed)
  {
    previousNote = currentNote;
    currentNote = getKeyboardNote(keyPressed);

    openGate();
    glide.begin(this->getBpmInMilliseconds(), portamento, previousNote.voltage, currentNote.voltage);

    if (isStepEditing())
    {
      setPatternNote(currentNote);
      currentStep = nextStep(currentStep);
      displayStep();
    }
  }

  void patternInsertRest()
  {
    Note note;
    note.isRest = true;
    note.octave = 0;
    note.pitch = 0;
    setPatternNote(note);
    currentStep = nextStep(currentStep);
    displayStep();
  }

  void update()
  {
    if (bpmClock.done())
    {
      bpmClock.cycle();
      bpmClockTick();
    }

    if (gateTimer.done())
      closeGate();
    if (clockLedTimer.done())
      clockLedOff();    

    dialog.update();
  }
};

#endif