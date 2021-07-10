#include <Arduino.h>
#include "memory.h"
#include "display.h"
#include "knob.h"
#include "controls.h"
#include "ImTimer.h"

enum LoadState:byte { MEM_cancel, MEM_bankSelect, MEM_patternSelect, MEM_confirmLoad, MEM_patternLoading, MEM_complete };
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
      case MEM_patternSelect:
      case MEM_confirmLoad:
      case MEM_patternLoading:
      case MEM_complete:
      case MEM_cancel:
        break;
    }    
  }
}