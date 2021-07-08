#ifndef MY_KNOB_H
#define MY_KNOB_H

#include <Arduino.h>
#include <AnalogMultiButton.h>
#include <RotaryEncoder.h>
#include "leds.h"

enum KnobFunction {
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
  int pos = 0;
  int rangeMin = 0;
  int rangeMax = 20;
  bool constrained = true;
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
    
    int lastDirection = 0;
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
      
      //log("k");
      //Serial.print(index);
      /*
      log("\tmode: ");
      Serial.print(modeIndex);
      log("\trange min: ");
      Serial.print(rangeMin[modeIndex]);
      log("\trange max: ");
      Serial.print(rangeMax[modeIndex]);
      log("\n");
      */
    
}
    void setLED() {
      for (byte i = 0; i < 3; i ++) {
        setLedState(knobModes[i], ledOFF); 
      }
      setLedState(knobModes[modeIndex], ledON);
      char buffer[30];
      sprintf(buffer, "knob %d set to mode %d\n", index, modeIndex);
      log(buffer);
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
        /*
        Serial.print("pos:");
        Serial.print(newPos);
        Serial.print(" dir:");
        Serial.println(lastDirection);
        */
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