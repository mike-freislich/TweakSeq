#ifndef MY_LOGGER
#define MY_LOGGER

#include <Arduino.h>

void log(const char* buffer) {
  #if (LOGGING)
  
  Serial.print(buffer);
  #endif
}

#endif