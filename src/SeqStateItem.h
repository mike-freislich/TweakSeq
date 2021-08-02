#ifndef SEQSTATEITEM_H
#define SEQSTATEITEM_H

#include <Arduino.h>

struct seqStateItem
{
    int16_t value;
    int16_t max;
    int16_t min;
};

#endif