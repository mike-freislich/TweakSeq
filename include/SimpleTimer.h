#ifndef SIMPLETIMER_H
#define SIMPLETIMER_H

#include <Arduino.h>

class SimpleTimer
{
private:
  uint32_t lastTime;
  
public:
  uint32_t timeout;
  bool running = false;
  
  SimpleTimer() {}
  SimpleTimer(uint32_t timeout) { start(timeout); }
  uint32_t elapsed() { return millis() - lastTime; }
  
  void cycle() { start(timeout);}
  
  void start(uint32_t timeout)
  {
    this->lastTime = millis();
    this->timeout = timeout;
    running = true;
  }

  void stop() { running = false; }
  
  bool done(bool restart = true)
  {    
    if (!running) return false;

    bool val = (millis() - lastTime >= timeout);
    if (val) {
      if (restart)
        start(timeout);
      else
        stop();
    }

    return val;
  }
};

#endif