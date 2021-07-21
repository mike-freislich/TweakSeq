#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include <Arduino.h>
#include <EEPROM.h>
#include "pattern.h"
#if (SHOWMEM)
#include "MemoryFree.h"
#endif

/* NOTE DATA SPEC
 * 0-95 pitch
 * midi offset = pitch + 23
 * 100  REST
 * 101  TIE //TODO: add tie support
 */

// Memory Banks
const uint8_t BANK_MAX = 4; // number of pattern banks
uint8_t memBank = 0;
uint8_t memPattern = 0;

const uint8_t REST = 100;
const uint8_t TIE = REST + 1;
const uint8_t MIDI_OFFSET = 23;
uint8_t pattern[] = {0, 12, 24, 36, 48, 60, 72, 84, 36, 39, 41, 39, 36, 40, 41, 95};

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
  Pattern p;
  for (byte b = 0; b < 16; b++) {
    p.note[b] = pattern[b];
  }
  uint16_t size = sizeof(p);
  Serial.print(F("Pattern bytes: "));
  Serial.println(size);
  showFreeMemory(100);

  p.setRest(0);
  p.setRest(1);
  p.setTie(0);
  p.setTie(1);
  p.setTie(2);

  Pattern a = newPatternFromBytes(p.bytes()); // TODO: fix problem with loading struct from bytes

  for (byte b = 0; b < sizeof(p); b++)
  {
    
    Serial.print(a.bytes()[b]);
    Serial.print("\t");
    Serial.print(p.bytes()[b]);
    Serial.println();    
  }
  Serial.println();
}

void savePattern(uint8_t toSlot, uint8_t inBank)
{
  uint16_t slotLocation = (toSlot * PATTERN_STEP_MAX) + (inBank * PATTERN_STEP_MAX * PATTERN_MAX);
  if ((slotLocation + sizeof(Pattern)) < EEPROM.length())
    for (uint8_t i = 0; i < PATTERN_STEP_MAX; i++)
      EEPROM.write(slotLocation + i, pattern[i]);
}

void loadPattern(uint8_t fromSlot, uint8_t inBank)
{
  int slotLocation = (fromSlot * PATTERN_STEP_MAX) + (inBank * PATTERN_STEP_MAX * PATTERN_MAX);
  for (uint8_t i = 0; i < PATTERN_STEP_MAX; i++)
    pattern[i] = EEPROM.read(slotLocation + i);
}

#endif