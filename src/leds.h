#ifndef MYLEDS
#define MYLEDS

#include <Arduino.h>
#include "ImTimer.h"
#include "logger.h"

const int DISP_HoldValue = 1000;

const int outputEnablePin = 3;  // Shift Register - pin 13
const int dataPin = 4;          // Shift Register - pin 14
const int latchPin = 5;         // Shift Register - pin 12
const int clockPin = 6;         // Shift Register - pin 11

const byte ledKnob[9] = {16,17,18,19,20,21,22,23,24};
const byte ledSHIFT = 25;
const byte ledPLAY = 31;
const byte ledENTER = 28;
const byte ledClock = 26;
const byte ledGate = 27;
const byte outClock = 29;
const byte outGate = 30;

unsigned long uiData = 0, flashData = 0;
bool flashState = false;
bool didDisplayUpdate = false;


enum LedState:byte {  ledOFF = 0, ledON = 1, ledFLASH = 2 };
enum DisplayMode { DM_SEQUENCE, DM_VALUE };
DisplayMode displayMode = DM_SEQUENCE;

ImTimer dialogTimer;
ImTimer flashTimer;


void setBrightness(byte brightness) // 0 to 255
{
  analogWrite(outputEnablePin, 255 - brightness);  
}

bool ioState(byte channel) {
  return (bitRead(uiData, channel) == 1);
}

void ioSet(byte channel, bool value) {
  didDisplayUpdate = true;
  
  if (value) {
    bitSet(uiData, channel);    
  } else {
    bitClear(uiData, channel);
  }  
}

void sendGateSignal(bool value) {
  ioSet(outGate, value); 
}

void sendClockSignal(bool value) {
  ioSet(outClock, value);  
}

void shiftOut(int myDataPin, int myClockPin, byte myDataOut)
{
  int i = 0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);
  for (i = 7; i >= 0; i--)  {
    digitalWrite(myClockPin, 0);
    if ( myDataOut & (1 << i) )
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
  byte dataA = data;
  byte dataB = data >> 8;
  byte dataC = data >> 16;
  byte dataD = data >> 24;
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
  for (byte i = 0; i < 16; i ++)
    ioSet(i, false);    
}

void setSequencerStep(byte step) {
  if (displayMode == DM_SEQUENCE) {
    clearSequenceLights();  
    ioSet(step%16, true);
  }
}

void setSequencerDisplay() {
  displayMode = DM_SEQUENCE;
  clearSequenceLights();
}

void setValuePicker(int value, int low, int high, int ms) {
  
  if (ms > 0)                                           // if zero or less... there's no timeout
    dialogTimer.start(once, ms, setSequencerDisplay);  // timeout 
  
  displayMode = DM_VALUE;
    
  clearSequenceLights(); 
  int steps = high - low; 
  int percentValue = (double)(value - low) / (double)steps * 16;

  if (percentValue > 15) percentValue = 15;
      
  for (byte i = 0; i <= percentValue; i ++) {
    ioSet(i, true);  
  } 
}



void hideDialog() {
  dialogTimer.stop();
  setSequencerDisplay();
}

void updateDisplay() {  
  dialogTimer.update();
  flashTimer.update();

  if (didDisplayUpdate) updateShiftRegister(uiData);
}

LedState getLedState(byte ledIndex) {

  LedState ledState = (LedState)ioState(ledIndex);
  
  if (bitRead(flashData, ledIndex))
    ledState = ledFLASH;  
    
  return ledState;  
}

void setLedState(byte ledIndex, LedState state) {
  if (ledIndex < 0 || ledIndex > 31) return;

  switch (state) {
    
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

LedState nextLedState(byte ledIndex) {
    LedState newState = static_cast<LedState>((getLedState(ledIndex) + 1) % 3);
    setLedState(ledIndex, newState);
    return newState;
}

void flashTimerTick() {
  flashState = !flashState;
  for (byte i = 0; i < 32; i ++) {
    if (bitRead(flashData, i)) {
      ioSet(i, flashState);
    }
  }
}

void setupLeds() {
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(outputEnablePin, OUTPUT);

  setBrightness(20);
  flashTimer.start(looping, 200, flashTimerTick);
}




#endif