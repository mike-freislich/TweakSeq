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
  uint8_t length=16;
  bool getTie(uint8_t position) { return bitRead(tieData, position); }
  void setTie(uint8_t position, bool isTie = true)
  {
    if (isTie)
      bitSet(tieData, position);
    else
      bitClear(tieData, position);
  }
  bool getRest(uint8_t position) { return bitRead(restData, position); }
  void setRest(uint8_t position, bool isRest = true)
  {
    if (isRest)
      bitSet(restData, position);
    else
      bitClear(restData, position);
  }
  uint8_t *bytes() { return (uint8_t *)this; }
  static Pattern fromBytes(uint8_t *data)
  {
    Pattern p = Pattern();
    memcpy(&p, data, sizeof(Pattern));
    return p;
  }
};

uint8_t *patternToBytes(Pattern *pattern)
{
  return (uint8_t *)pattern;
}

Pattern newPatternFromBytes(uint8_t *bytes)
{
  Pattern p;
  uint8_t *data = (uint8_t *)&p;
#if (LOGGING)
  Serial.print("pattern size: ");
  Serial.println(sizeof(Pattern));
#endif

  for (byte i = 0; i < sizeof(Pattern); i++)
    data[i] = bytes[i];

  return p;
}

#endif