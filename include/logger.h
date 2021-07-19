#ifndef MY_LOGGER
#define MY_LOGGER

#include <Arduino.h>

void log(const char *message)
{
    Serial.print(message);
    #if (DEBUG)
    
    #endif
}

#endif