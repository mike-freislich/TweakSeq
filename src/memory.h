#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include <Arduino.h>
#include <EEPROM.h>
#include "pattern.h"
#if (SHOWMEM)
#include "MemoryFree.h"
#endif

/// NOTE DATA SPEC
/// TODO: add tie support

// Memory Banks
const uint8_t BANK_MAX = 4; // number of pattern banks
uint8_t memBank = 0;
uint8_t memPattern = 0;

const uint8_t REST = 100;
const uint8_t TIE = REST + 1;
const uint8_t MIDI_OFFSET = 23;
uint8_t _pattern[] = {0, 12, 24, 36, 48, 60, 72, 84, 36, 39, 41, 39, 36, 40, 41, 95};
Pattern pattern = Pattern();

enum StorageAction
{
  LOAD_PATTERN,
  SAVE_PATTERN
};

void showFreeMemory(uint8_t i = 99)
{
#if (SHOWMEM)
  Serial.print(F("freemem["));
  Serial.print(i);
  Serial.print(F("]="));
  Serial.println(freeMemory());
#endif
}

void testPattern()
{
  
}

void savePattern(uint8_t toSlot, uint8_t inBank)
{
  uint8_t patternSize = sizeof(Pattern);
  uint16_t slotLocation = (toSlot * patternSize) + (inBank * patternSize * PATTERN_MAX);  
  
  uint8_t *data = pattern.bytes();
  if ((slotLocation + patternSize) < EEPROM.length()) 
    for (uint8_t i = 0; i < patternSize; i++)
      EEPROM.write(slotLocation + i, data[i]);
}

void loadPattern(uint8_t fromSlot, uint8_t inBank)
{  
  int slotLocation = (fromSlot * sizeof(Pattern)) + (inBank * sizeof(Pattern) * PATTERN_MAX);
  
  for (uint8_t i = 0; i < sizeof(Pattern); i++)
    ((uint8_t *)&pattern)[i] = EEPROM.read(slotLocation + i);
}

#endif