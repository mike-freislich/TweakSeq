#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include <Arduino.h>
#include <EEPROM.h> //for reading and writing patterns to permanent memory


/* NOTE DATA SPEC
 * 0-95 pitch
 * midi offset = pitch + 23
 * 100  REST
 * 101  TIE
 */

// BANKS
const byte BANK_MAX = 4;            // number of pattern banks
byte bank = 0;

// PATTERNS
const byte PATTERN_MAX = 8;         // number of patterns
const byte PATTERN_STEP_MAX = 16;   // number of steps per pattern
byte currentPattern = 0;


const byte REST = 100;
const byte TIE  = REST + 1;
const byte MIDI_OFFSET = 23;
byte pattern[] = {0, 12, 24, 36, 48, 60, 72, 84, 36, 39, 41, 39, 36, 40, 41, 95};

void savePattern(uint16_t toSlot, byte inBank) {
  int slotLocation = (toSlot * PATTERN_STEP_MAX) + (inBank * PATTERN_STEP_MAX * PATTERN_MAX);
  int offset;
  for (int i = 0; i < PATTERN_STEP_MAX; i++) {
    if (offset < EEPROM.length()) {
      EEPROM.write(slotLocation + i, pattern[i]);
    }
  }
}

void loadPattern(byte fromSlot, byte inBank) {
  int slotLocation = (fromSlot * PATTERN_STEP_MAX) + (inBank * PATTERN_STEP_MAX * PATTERN_MAX);
  for (byte i = 0; i < PATTERN_STEP_MAX; i++)
    pattern[i] = EEPROM.read(slotLocation + i);
}

#endif