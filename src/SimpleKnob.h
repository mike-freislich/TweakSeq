#ifndef SIMPLEKNOB_H
#define SIMPLEKNOB_H

#include <arduino.h>
#include "RotaryEncoder.h"

class SimpleKnob
{
private:
    RotaryEncoder *_encoder;
    int8_t _value;
    bool _changed;

public:
    SimpleKnob(uint8_t pin1, uint8_t pin2)
    {
        _encoder = new RotaryEncoder(pin1, pin2, RotaryEncoder::LatchMode::TWO03);
        _value = 0;
    }

    ~SimpleKnob()
    {
        if (_encoder)
            delete (_encoder);
    }

    void setValue(int8_t value) {
        _value = value;
        _encoder->setPosition(value);
    }

    bool didValueChange() { return _changed; }

    void update()
    {
        _encoder->tick();
        int8_t direction = (int8_t)_encoder->getDirection();
        _value += direction;
        _changed = direction != 0;
    }
};

#endif