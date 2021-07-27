#ifndef NOTE_H
#define NOTE_H

#include <Arduino.h>

struct Note {
  uint8_t stepNumber = 0;
  uint8_t octave = 1;
  uint16_t pitch = 1;
  uint16_t voltage = 0;
  uint8_t midiNote = 0;
  bool isRest = false, isTie = false;
};

#endif