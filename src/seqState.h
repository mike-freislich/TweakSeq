#ifndef SEQSTATE
#define SEQSTATE

#include "SeqStateItem.h"

class SeqState
{
private:
    seqStateItem _items[2][3][3]; // shift, knob, setting

public:
    SeqState() { }

    seqStateItem *item(uint8_t shift, uint8_t knob, uint8_t index) { return &_items[shift][knob][index]; }
};

#endif
