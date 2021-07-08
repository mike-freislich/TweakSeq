#ifndef MY_KNOB_H
#define MY_KNOB_H

#include <Arduino.h>
#include <AnalogMultiButton.h>
#include <RotaryEncoder.h>
#include "leds.h"

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
  byte pos = 0;
  byte rangeMin = 0;
  byte rangeMax = 20;  
};

class Knob {
  private:
    AnalogMultiButton* amButton;
    RotaryEncoder* encoder;
    KnobFunction* knobModes;    
    byte index = 0;    
    byte modeIndex = 0;
    byte modeShift() { return getLedState(ledSHIFT);}

    KnobState knobState[3][3];
    
    byte lastDirection = 0;
    bool changed = false;

    int constrainPos(int pos, int rangeMin, int rangeMax) {
      int newPos = pos;
      if (pos > rangeMax) newPos = rangeMax;        
      if (pos < rangeMin) newPos = rangeMin;
      return newPos;    
    }

    KnobState* getKnobState(byte mode) {
      return &knobState[modeShift()][mode];
    }
    
  public:
    Knob(byte index, AnalogMultiButton amButton, int pin1, int pin2) {
      encoder = new RotaryEncoder(pin2, pin1, RotaryEncoder::LatchMode::FOUR3);
      this->index = index;
      this->amButton = &amButton;
    }

    void setRange(LedState shift, int forMode, int newRangeMin, int newRangeMax) {      
      KnobState* k = &knobState[shift][forMode];
      k->rangeMin = newRangeMin;
      k->rangeMax = newRangeMax;
      k->pos = constrainPos(value(), k->rangeMin, k->rangeMax);      
    }

    void setValue(int newValue) {
      KnobState* k = getKnobState(modeIndex);
      int newPos = constrainPos(newValue, k->rangeMin, k->rangeMax);
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

    int value() {
      return getKnobState(modeIndex)->pos;
    }

    void addModes(KnobFunction* modes) {
      this->knobModes = modes;
    }

    void update() {
      encoder->tick();
      KnobState* k = getKnobState(modeIndex);
      int newPos = constrainPos(encoder->getPosition(), k->rangeMin, k->rangeMax);
      lastDirection = (int)encoder->getDirection();
      changed = lastDirection != 0;

      if (newPos != encoder->getPosition()) {
        encoder->setPosition(newPos);
      }

      if (k->pos != newPos) {
        k->pos = newPos;        
      }           
    }

    int direction() { return lastDirection; }
    bool onPress() { return amButton->onPress(this->index-1); }

    bool didChange() {
      bool c = changed;
      changed = false;
      return c;
    }

    int getRangeMin() { return getKnobState(modeIndex)->rangeMin; }
    int getRangeMax() { return getKnobState(modeIndex)->rangeMax; }

    byte getIndex() { return index; }
};

Knob* knob[3];

#endif