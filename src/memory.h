#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include <Arduino.h>
#include <EEPROM.h>

/* NOTE DATA SPEC
 * 0-95 pitch
 * midi offset = pitch + 23
 * 100  REST
 * 101  TIE //TODO: add tie support
 */

// BANKS
const uint8_t BANK_MAX = 4; // number of pattern banks
uint8_t memBank = 0;

// PATTERNS
const uint8_t PATTERN_MAX = 8;       // number of patterns
const uint8_t PATTERN_STEP_MAX = 16; // number of steps per pattern
uint8_t memPattern = 0;

const uint8_t REST = 100;
const uint8_t TIE = REST + 1;
const uint8_t MIDI_OFFSET = 23;
uint8_t pattern[] = {0, 12, 24, 36, 48, 60, 72, 84, 36, 39, 41, 39, 36, 40, 41, 95};

struct Pattern
{
  uint8_t note[16];
  uint16_t tieData;
  uint16_t restData;
  bool getTie(uint8_t position) { return bitRead(tieData, position); }
  void setTie(uint8_t position) { bitSet(tieData, position); }
  bool getRest(uint8_t position) { return bitRead(restData, position); }  
  void setRest(uint8_t position) { bitSet(restData, position); }  
} __attribute__((__packed__));

enum StorageAction
{
  LOAD_PATTERN,
  SAVE_PATTERN
};

void testPattern()
{
  Pattern p;
  uint16_t size = sizeof(p);
  Serial.print(F("Pattern: "));
  Serial.println(size);
  //bool rest = p.restAtIndex(2);
}

void savePattern(uint8_t toSlot, uint8_t inBank)
{
  int slotLocation = (toSlot * PATTERN_STEP_MAX) + (inBank * PATTERN_STEP_MAX * PATTERN_MAX);
  uint16_t offset;
  for (int i = 0; i < PATTERN_STEP_MAX; i++)
  {
    if (offset < EEPROM.length())
    {
      EEPROM.write(slotLocation + i, pattern[i]);
    }
  }
}

void loadPattern(uint8_t fromSlot, uint8_t inBank)
{
  int slotLocation = (fromSlot * PATTERN_STEP_MAX) + (inBank * PATTERN_STEP_MAX * PATTERN_MAX);
  for (uint8_t i = 0; i < PATTERN_STEP_MAX; i++)
    pattern[i] = EEPROM.read(slotLocation + i);
}

#endif