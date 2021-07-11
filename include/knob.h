#ifndef MY_KNOB_H
#define MY_KNOB_H

#include <Arduino.h>
#include <AnalogMultiButton.h>
#include <RotaryEncoder.h>
#include "display.h"

enum KnobFunction:byte {
  TempoAdjust   = 16,
  StepSelect,
  GateTime,
  PlayMode,
  GlideTime,
  Pitch,
  NumSteps,
  GlideShape,
  Octave
  };

struct KnobState {
  short pos = 0;
  short rangeMin = 0;
  short rangeMax = 20;  
} __attribute__ (( __packed__ ));

class Knob {
  private:
    AnalogMultiButton* amButton;
    RotaryEncoder* encoder;
    KnobFunction* knobModes;    
    byte index = 0;    
    byte modeIndex = 0;
    byte modeShift() {
    
      byte value = getLedState(ledSHIFT);
      if (value > 1) value = 1;
      return value;
      }

    KnobState knobState[2][3];
    
    short lastDirection = 0;
    bool changed = false;

    short constrainPos(short pos, short rangeMin, short rangeMax) {
      short newPos = pos;
      if (pos > rangeMax) newPos = rangeMax;        
      if (pos < rangeMin) newPos = rangeMin;
      return newPos;    
    }

    KnobState* getKnobState(byte mode) {
      return &knobState[modeShift()][mode];
    }
    
  public:
    Knob(byte index, AnalogMultiButton amButton, byte pin1, byte pin2) {
      encoder = new RotaryEncoder(pin2, pin1, RotaryEncoder::LatchMode::FOUR3);
      this->index = index;
      this->amButton = &amButton;
    }

    void setRange(LedState shift, byte forMode, short newRangeMin, short newRangeMax) {      
      KnobState* k = &knobState[shift][forMode];
      k->rangeMin = newRangeMin;
      k->rangeMax = newRangeMax;
      k->pos = constrainPos(value(), k->rangeMin, k->rangeMax);      
    }

    void setValue(short newValue) {
      KnobState* k = getKnobState(modeIndex);
      short newPos = constrainPos(newValue, k->rangeMin, k->rangeMax);
      encoder->setPosition(newPos);
      k->pos = newPos;     
    }
        
    void nextMode() {
      modeIndex++;
      modeIndex = modeIndex%3;
      setMode(modeIndex);     
    }

    void setMode(byte mode) {      
      modeIndex = mode % 3;           
      encoder->setPosition(getKnobState(modeIndex)->pos);
      setLED();                
}
    void setLED() {
      for (byte i = 0; i < 3; i ++) {
        setLedState(knobModes[i], ledOFF); 
      }
      setLedState(knobModes[modeIndex], ledON);
    }

    byte getMode() {
      return modeIndex;
    }

    short value() {
      return getKnobState(modeIndex)->pos;
    }

    void addModes(KnobFunction* modes) {
      this->knobModes = modes;
    }

    void update() {
      encoder->tick();
      changed = false;
      KnobState* k = getKnobState(modeIndex);
      short newPos = constrainPos(encoder->getPosition(), k->rangeMin, k->rangeMax);
      lastDirection = (short)encoder->getDirection();      

      if (newPos != encoder->getPosition()) {
        encoder->setPosition(newPos);
      }

      if (k->pos != newPos) {
        k->pos = newPos;      
        changed = true;  
      }           
    }

    short direction() { return lastDirection; }
    bool onPress() { return amButton->onPress(this->index-1); }

    bool didChange() {
      bool c = changed;
      changed = false;
      return c;
    }

    short getRangeMin() { return getKnobState(modeIndex)->rangeMin; }
    short getRangeMax() { return getKnobState(modeIndex)->rangeMax; }

    byte getIndex() { return index; }
};

Knob* knob[3];

#endif