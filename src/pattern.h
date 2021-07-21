#ifndef MY_PATTERN
#define MY_PATTERN

#include <Arduino.h>

const uint8_t PATTERN_MAX = 8;       // number of patterns
const uint8_t PATTERN_STEP_MAX = 16; // number of steps per pattern

struct Pattern
{
  uint8_t note[16];
  uint16_t tieData;
  uint16_t restData;
  bool getTie(uint8_t position) { return bitRead(tieData, position); }
  void setTie(uint8_t position) { bitSet(tieData, position); }
  bool getRest(uint8_t position) { return bitRead(restData, position); }
  void setRest(uint8_t position) { bitSet(restData, position); }
  uint8_t *bytes() { return (uint8_t *)this; }
} __attribute__((__packed__));


Pattern newPatternFromBytes(uint8_t *b)
{
  Pattern p;
  uint8_t *data = (uint8_t *)&p;
  for (byte i = 0; i < sizeof(p); i ++)  
    data[i] = b[i];  
  
  return p;
}

#endif