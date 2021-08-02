#ifndef SEQSTATEITEM_H
#define SEQSTATEITEM_H

#include <Arduino.h>

struct seqStateItem
{
    int16_t value;
    int16_t max;
    int16_t min;
    uint8_t steps;
    void set(int16_t min, int16_t max, uint8_t steps = 0, int16_t value = 0)
    {
        if (steps == 0)
            this->steps = max - min + 1;
        this->max = max;
        this->min = min;
        this->value = value;
    }

};

#endif