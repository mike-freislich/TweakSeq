#ifndef MY_LOGGER
#define MY_LOGGER

#include <Arduino.h>

void log(char* buffer) {
  #if (LOGGING)
  Serial.print(buffer);
  #endif
}

#endif