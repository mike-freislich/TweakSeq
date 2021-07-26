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
  for (uint8_t x = 0; x < 16; x++)
    p.note[x] = pattern[x];
  p.tieData = 0;
  p.restData = 0;
  //p.setRest(0);
  //p.setRest(1);
  //p.setTie(0);
  //p.setTie(1);
  //p.setTie(2);  

  // TODO: fix problem with loading struct from bytes
  Pattern a;
  memcpy(&a, &p, sizeof(Pattern));  

     #if (LOGGING)
  for (byte x = 0; x < sizeof(Pattern); x++)
  {
    Serial.print( ((uint8_t *)&p)[x]);
    Serial.print("\t");
    Serial.print( ((uint8_t *)&a)[x]);
    Serial.println();    
  }
  Serial.println();
  #endif
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