#ifndef MY_DISPLAY_H
#define MY_DISPLAY_H

#include <Arduino.h>
#include "ImTimer.h"
#include "dialog.h"
#include "uistate.h"



#pragma region GLOBALS
uint32_t uiData = 0, flashData = 0;
bool flashState = false;
bool didDisplayUpdate = false;
Dialog *dialog;

ImTimer dialogTimer;
ImTimer flashTimer;
#pragma endregion

#pragma region FUNCTION DECLARATIONS
void setBrightness(uint8_t brightness);
bool ioState(uint8_t channel);
void ioSet(uint8_t channel, bool value);
void sendGateSignal(bool value);
void sendClockSignal(bool value);
void shiftOut(int myDataPin, int myClockPin, uint8_t myDataOut);
void updateShiftRegister(unsigned long data);
void clearSequenceLights();
void setSequencerStep(uint8_t step);
void setSequencerDisplay();
void hideDialog();
void setValuePicker(int16_t value, int16_t low, int16_t high, bool timed, uint16_t ms);
void updateDisplay();
LedState getLedState(uint8_t ledIndex);
void setLedState(uint8_t ledIndex, LedState state);
LedState nextLedState(uint8_t ledIndex);
void flashTimerTick();
void setupDisplay();
#pragma endregion

void setBrightness(uint8_t brightness) // 0 to 255
{
  analogWrite(outputEnablePin, 255 - brightness);
}

bool ioState(uint8_t channel)
{
  return (bitRead(uiData, channel) == 1);
}

void ioSet(uint8_t channel, bool value)
{
  didDisplayUpdate = true;

  if (value)  
    bitSet(uiData, channel);
  else  
    bitClear(uiData, channel);
}

void sendGateSignal(bool value)
{
  ioSet(outGate, value);
}

void sendClockSignal(bool value)
{
  ioSet(outClock, value);
}

void shiftOut(int myDataPin, int myClockPin, uint8_t myDataOut)
{
  int i = 0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);
  for (i = 7; i >= 0; i--)
  {
    digitalWrite(myClockPin, 0);
    if (myDataOut & (1 << i))
      pinState = 1;
    else
      pinState = 0;

    digitalWrite(myDataPin, pinState);
    digitalWrite(myClockPin, 1);
    digitalWrite(myDataPin, 0);
  }
  digitalWrite(myClockPin, 0);
}

void updateShiftRegister(unsigned long data)
{
  uint8_t dataA = data;
  uint8_t dataB = data >> 8;
  uint8_t dataC = data >> 16;
  uint8_t dataD = data >> 24;
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, dataD);
  shiftOut(dataPin, clockPin, dataC);
  shiftOut(dataPin, clockPin, dataB);
  shiftOut(dataPin, clockPin, dataA);
  digitalWrite(latchPin, HIGH);

  didDisplayUpdate = false;
}

void clearSequenceLights()
{
  uiData &= (uint32_t)0xFFFF0000;  
  flashData &= (uint32_t)0xFFFF0000;
  didDisplayUpdate = true;
  //Serial.println(F("clear sequence lights"));
}

void setSequencerStep(uint8_t step)
{
  if (!dialog->isVisible())
  {
    clearSequenceLights();
    setLedState(step % 16, ledON);
  }
}

void hideDialog()
{
  dialog->hide();
  clearSequenceLights();     
}

void setValuePicker(int16_t value, int16_t low, int16_t high, bool timed = true, uint16_t ms = DIALOG_TIMEOUT)
{       
  dialog->setDisplayValue(value, low, high, timed, ms);
  dialog->show();
  dialog->writeoutDisplayBuffer(&uiData, &flashData);
  didDisplayUpdate = true;
}

void updateDisplay()
{
  flashTimer.update();
  dialog->update();

  if (didDisplayUpdate) 
    updateShiftRegister(uiData);      
}

LedState getLedState(uint8_t ledIndex)
{
  LedState ledState = (LedState)ioState(ledIndex);

  if (bitRead(flashData, ledIndex))
    ledState = ledFLASH;

  return ledState;
}

void setLedState(uint8_t ledIndex, LedState state)
{
  if (ledIndex < 0 || ledIndex > 31)
    return;

  switch (state)
  {

  case ledOFF:
    ioSet(ledIndex, false);
    bitClear(flashData, ledIndex);
    break;

  case ledON:
    ioSet(ledIndex, true);
    bitClear(flashData, ledIndex);
    break;

  case ledFLASH:
    bitSet(flashData, ledIndex);
    break;
  }
}

LedState nextLedState(uint8_t ledIndex)
{
  LedState newState = static_cast<LedState>((getLedState(ledIndex) + 1) % 3);
  setLedState(ledIndex, newState);
  return newState;
}

void flashTimerTick()
{
  flashState = !flashState;
  //uiData ^= flashData;
  didDisplayUpdate = true;
  
  
  for (uint8_t i = 0; i < 32; i++)
  {
    if (bitRead(flashData, i))
      ioSet(i, flashState);
  }  
}

void setupDisplay()
{
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(outputEnablePin, OUTPUT);

  setBrightness(20);
  flashTimer.start(looping, 200, flashTimerTick);
  dialog = new Dialog(&dialogTimer, hideDialog); 
}

#endif