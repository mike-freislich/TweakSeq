#ifndef MY_TIMER_H
#define MY_TIMER_H
#include <Arduino.h>
#include <util/atomic.h>


void setMillis(unsigned long ms)
{
    extern unsigned long timer0_millis;
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        timer0_millis = ms;
    }
}

enum ImTimerState {
  looping = 1, once = 2, stopped = 3
};

class ImTimer {
  private:
    unsigned long lastTick;
    unsigned long duration;
    ImTimerState state = stopped, prevState = looping;

    void (*tickHandler)();

    void init(ImTimerState state, unsigned long duration, void (*tickHandler)()) {            
      this->prevState = this->state;
      this->lastTick = millis();
      this->duration = duration;
      this->state = state;
      this->tickHandler = tickHandler;
    }

  public:
    
    ImTimer() {}
    ImTimer(void (*tickHandler)()) {
      setTickHandler(tickHandler);
    }

    bool isRunning() { return state == once || state == looping; }

    void setTickHandler(void (*tickHandler)()) {
      this->tickHandler = tickHandler;
    }

    void update() {
      if (state == stopped) return; 

      unsigned long now = millis();
      unsigned long elapsed = now - lastTick;

      if (elapsed > duration) {
        if (state == once) stop();  
        lastTick = now;             
        tickHandler();                 
      }
    }
    
    void stop() {
      prevState = state;
      state = stopped;
    }

    void start(ImTimerState state, unsigned long duration) {
      init(state, duration, tickHandler);  
    }
    
    void start(unsigned long duration, void (*tickHandler)()) {
      init(prevState, duration, tickHandler);
    }
 
    void start(ImTimerState state, unsigned long duration, void (*tickHandler)()) {       
      init(state, duration, tickHandler);  
    }

    void changeDuration(unsigned long duration) {
      this->duration = duration;
    }
};

#endif