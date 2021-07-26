#ifndef SIMPLETIMER_H
#define SIMPLETIMER_H

#include <Arduino.h>

class SimpleTimer
{
private:
  uint32_t lastTime;
  uint32_t timeout;

public:
  SimpleTimer(uint32_t timeout) { start(timeout); }
  uint32_t elapsed() { return millis() - lastTime; }
  void cycle() { start(timeout); }
  
  void start(uint32_t timeout)
  {
    this->lastTime = millis();
    this->timeout = timeout;
  }
  
  bool done(bool restart = true)
  {    
    bool val = (millis() - lastTime >= timeout);
    if (val & restart)
      start(timeout);
    return val;
  }
};

#endif