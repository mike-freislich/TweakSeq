#ifndef MY_UISTATE
#define MY_UISTATE

#include <Arduino.h>

#pragma region CONSTANTS / ENUMS
const uint8_t outputEnablePin = 3; // Shift Register - pin 13
const uint8_t dataPin = 4;         // Shift Register - pin 14
const uint8_t latchPin = 5;        // Shift Register - pin 12
const uint8_t clockPin = 6;        // Shift Register - pin 11

const uint8_t ledKnob[] PROGMEM = {16, 17, 18, 19, 20, 21, 22, 23, 24};
const uint8_t ledSHIFT = 25;
const uint8_t ledPLAY = 31;
const uint8_t ledENTER = 28;
const uint8_t ledClock = 26;
const uint8_t ledGate = 27;
const uint8_t outClock = 29;
const uint8_t outGate = 30;

#pragma endregion

enum UIState
{
    SEQUENCER,
    ACTION_BANK_SELECT,
    ACTION_PATTERN_SELECT,
    ACTION_COMPLETE,
};

UIState uiState = UIState::SEQUENCER;
UIState uiLastState = uiState;

bool uiStateChanged() {
    bool changed = uiLastState != uiState;
    uiLastState = uiState;
    return changed;
}

enum LedState : uint8_t
{
  ledOFF = 0,
  ledON = 1,
  ledFLASH = 2
};

enum DisplayMode
{
  DM_SEQUENCE,
  DM_VALUE
};



#endif