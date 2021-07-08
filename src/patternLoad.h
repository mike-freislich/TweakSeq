#include <Arduino.h>
#include "memory.h"
#include "leds.h"
#include "knob.h"
#include "controls.h"
#include "ImTimer.h"

enum LoadState { MEM_cancel, MEM_bankSelect, MEM_patternSelect, MEM_confirmLoad, MEM_patternLoading, MEM_complete };
LoadState loadState = MEM_complete;

void plUpdate() {
  bpmClock.update();
  gateTimer.update();
  clockLedTimer.update();
  funcButtons.update();
  pianoBlack.update();
  pianoWhite.update();   
  
  updateDisplay();
}

void bankSelect() {
  // display select bank
    while (true)
        plUpdate();

    loadState = MEM_patternSelect;
}

void cancelPatternLoad() {
  
  loadState = MEM_complete;
}

void startLoadPattern() {
  loadState = MEM_bankSelect;

  while (loadState != MEM_complete || MEM_cancel) {
    switch (loadState) {
      case MEM_bankSelect:
        bankSelect();        
        break;
      case MEM_patternSelect:
        break;
      case MEM_confirmLoad:
        break;
      case MEM_patternLoading:
        break;
      case MEM_complete:
        break;
    }    
  }
}