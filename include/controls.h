#ifndef MY_CONTROLS_H
#define MY_CONTROLS_H
#include <Arduino.h>
#include <AnalogMultiButton.h>
#include <RotaryEncoder.h>
//#include "knob.h"


// ANALOG
const uint8_t BUTTONS_ENCODER    = A4; 
const uint8_t BUTTONS_KBD_BLACK  = A5; 
const uint8_t BUTTONS_KBD_WHITE  = A6; 
const uint8_t BUTTONS_FUNC       = A7; 

// DIGITAL
const uint8_t KNOB1_A      = A0; // knob1_pin_A
const uint8_t KNOB1_B      = A1; // knob1_pin_B
const uint8_t KNOB2_A      = A2; // knob2_pin_A
const uint8_t KNOB2_B      = A3; // knob2_pin_B
const uint8_t KNOB3_A      = 8; // knob3_pin_A
const uint8_t KNOB3_B      = 9; // knob3_pin_B

const uint8_t CLK_IN   = 2;    // External Clock

void setupIO() {
  pinMode(BUTTONS_ENCODER, INPUT);
  pinMode(BUTTONS_FUNC, INPUT);
  pinMode(BUTTONS_KBD_BLACK, INPUT);
  pinMode(BUTTONS_KBD_WHITE, INPUT);
  pinMode(CLK_IN, INPUT_PULLUP);
}

const uint8_t pitchIndexWhite[7] = { 0, 2, 4, 5, 7, 9, 11 };
const uint8_t pitchIndexBlack[5] = { 1, 3, 6, 8, 10 };

const int8_t ENC_BUTTONS_PIN = BUTTONS_ENCODER;
const int8_t ENC_BUTTONS_TOTAL = 3;
const int ENC_BUTTONS_VALUES[ENC_BUTTONS_TOTAL] = {0, 340, 511};
AnalogMultiButton encoderButtons(ENC_BUTTONS_PIN, ENC_BUTTONS_TOTAL, ENC_BUTTONS_VALUES);

enum FUNCTIONS:uint8_t { SHIFT = 0, PLAY = 1, LOAD = 2, SAVE = 3, ENTER = 4};
const int8_t FUNC_BUTTONS_PIN = BUTTONS_FUNC;
const int8_t FUNC_BUTTONS_TOTAL = 5;
const int FUNC_BUTTONS_VALUES[FUNC_BUTTONS_TOTAL] = {0, 339, 510, 613, 682};
AnalogMultiButton funcButtons(FUNC_BUTTONS_PIN, FUNC_BUTTONS_TOTAL, FUNC_BUTTONS_VALUES);

const int8_t KBDB_BUTTONS_PIN = BUTTONS_KBD_BLACK;
const int8_t KBDB_BUTTONS_TOTAL = 5;
const int KBDB_BUTTONS_VALUES[KBDB_BUTTONS_TOTAL] = {0, 236, 384, 486, 560};
AnalogMultiButton pianoBlack(KBDB_BUTTONS_PIN, KBDB_BUTTONS_TOTAL, KBDB_BUTTONS_VALUES);

const int8_t KBDW_BUTTONS_PIN = BUTTONS_KBD_WHITE;
const int8_t KBDW_BUTTONS_TOTAL = 7;
const int KBDW_BUTTONS_VALUES[KBDW_BUTTONS_TOTAL] = {0, 236, 384, 486, 560, 616, 660};
AnalogMultiButton pianoWhite(KBDW_BUTTONS_PIN, KBDW_BUTTONS_TOTAL, KBDW_BUTTONS_VALUES);


#endif