#ifndef MY_KNOB_H
#define MY_KNOB_H

#include <Arduino.h>
#include <AnalogMultiButton.h>
#include <RotaryEncoder.h>
#include "display.h"

enum KnobFunction : uint8_t
{
  TempoAdjust = 16,
  StepSelect,
  GateTime,
  PlayMode,
  GlideTime,
  Pitch,
  NumSteps,
  GlideShape,
  Octave
};

struct KnobState
{
  short pos = 0;
  short rangeMin = 0;
  short rangeMax = 20;
} __attribute__((__packed__));

class Knob
{
private:
  AnalogMultiButton *amButton;
  RotaryEncoder *encoder;
  KnobFunction *knobModes;
  uint8_t index = 0;
  uint8_t modeIndex = 0;
  uint8_t lastShiftState = 0;
  bool changed = false;
  short lastDirection = 0;  
  KnobState knobState[2][3];  
  
  uint8_t modeShift()
  {
    uint8_t value = getLedState(ledSHIFT);
    value = (value > 1) ? 1 : value;    
    if (value != lastShiftState) {
      lastShiftState = value;       
      resetKnobState();      
    }
    return value;
  }

  short constrainPos(short pos, short rangeMin, short rangeMax)
  {
    short newPos = constrain(pos, rangeMin, rangeMax);
    
    if (pos > rangeMax)
      newPos = rangeMax;
    if (pos < rangeMin)
      newPos = rangeMin;
    return newPos;
  }

  KnobState *getKnobState(uint8_t mode)
  {
    return &knobState[modeShift()][mode];
  }

public:

  Knob(uint8_t index, AnalogMultiButton amButton, uint8_t pin1, uint8_t pin2)
  {
    encoder = new RotaryEncoder(pin2, pin1, RotaryEncoder::LatchMode::FOUR3);
    this->index = index;
    this->amButton = &amButton;
  }

  void setRange(LedState shift, uint8_t forMode, short newRangeMin, short newRangeMax)
  {
    KnobState *k = &knobState[shift][forMode];
    k->rangeMin = newRangeMin;
    k->rangeMax = newRangeMax;
    k->pos = constrainPos(value(), k->rangeMin, k->rangeMax);
  }

  void setValue(short newValue)
  {
    KnobState *k = getKnobState(modeIndex);
    short newPos = constrainPos(newValue, k->rangeMin, k->rangeMax);
    encoder->setPosition(newPos);
    k->pos = newPos;
  }

  void nextMode()
  {
    modeIndex++;
    modeIndex = modeIndex % 3;
    setMode(modeIndex);
  }

  void setMode(uint8_t mode)
  {
    modeIndex = mode % 3;
    encoder->setPosition(getKnobState(modeIndex)->pos);
    setLED();
  }
  
  void setLED()
  {
    for (uint8_t i = 0; i < 3; i++)
    {
      setLedState(knobModes[i], ledOFF);
    }
    setLedState(knobModes[modeIndex], ledON);
  }

  uint8_t getMode()
  {
    return modeIndex;
  }

  short value()
  {
    return getKnobState(modeIndex)->pos;
  }

  void addModes(KnobFunction *modes)
  {
    this->knobModes = modes;
  }

  void resetKnobState() {
    changed=false;
    KnobState *k = getKnobState(modeIndex);
    k->pos = constrainPos(encoder->getPosition(), k->rangeMin, k->rangeMax);
    lastDirection = (short)encoder->getDirection();    
  }

  void update()
  {
    encoder->tick();
    changed = false;
    KnobState *k = getKnobState(modeIndex);
    short newPos = constrainPos(encoder->getPosition(), k->rangeMin, k->rangeMax);
    lastDirection = (short)encoder->getDirection();

    if (newPos != encoder->getPosition())    
      encoder->setPosition(newPos);    

    if (k->pos != newPos)
    {
      k->pos = newPos;
      changed = true;
    }
  }

  short direction() { return lastDirection; }
  bool onPress() { return amButton->onPress(this->index - 1); }

  bool didChange()
  {
    bool c = changed;
    changed = false;
    return c;
  }

  short getRangeMin() { return getKnobState(modeIndex)->rangeMin; }
  short getRangeMax() { return getKnobState(modeIndex)->rangeMax; }

  uint8_t getIndex() { return index; }
};

Knob *knob[3];

#endif