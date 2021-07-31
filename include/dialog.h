#ifndef MY_DIALOG
#define MY_DIALOG

#include "SimpleTimer.h"

const uint16_t DIALOG_TIMEOUT = 1000;

class Dialog
{
private:
    uint16_t displayData;
    uint16_t flashData;

    void addDisplayBorders()
    {
        if (value == low || (low < 0 && value == 0))
            flashData = flashData | 0x03; // set flash led 1 + 2
        displayData = displayData | 0x03; // set on led 1 + 2

        if ((value == high) || (low < 0 && value == 0))
            flashData = flashData | 0xC000; // set flash led 15 + 16
        displayData = displayData | 0xC000; // set on led 15 + 16
    }

    void clearDisplayBuffer()
    {
        displayData = 0;
        flashData = 0;
    }

protected:
    SimpleTimer dialogTimer = SimpleTimer(DIALOG_TIMEOUT);
    uint16_t timeout;
    bool timed, visible;
    bool _didClose = false;

public:
    int16_t value;
    int16_t low;
    int16_t high;

    Dialog() {}

    virtual ~Dialog() {}

    bool isVisible() { return this->visible; }

    void show()
    {
        if (timed)
            dialogTimer.start(timeout);
        else
            dialogTimer.stop();

        visible = true;
        _didClose = false;
    }

    void hide()
    {
        if (visible)
        {
            visible = false;
            _didClose = true;
        }
        ShiftRegisterPWM::singleton->clearSequenceLights();
    }

    void update()
    {
        if (timed && dialogTimer.done() && visible)
            hide();
    }

    void setDisplayValue(int16_t value, int16_t low, int16_t high, bool timed, uint16_t timeout)
    {
        this->timeout = timeout;
        this->value = value;
        this->low = low;
        this->high = high;
        this->timed = timed;

        ShiftRegisterPWM::singleton->clearSequenceLights();

        bufferDisplay();
        show();
    }

    virtual void bufferDisplay()
    {
        clearDisplayBuffer();

        uint16_t percentValue;
        uint16_t steps = high - low + 1;

        if (steps <= 10 || steps == 16)
            percentValue = (low == 0) ? value + 1 : value;
        else
        {
            percentValue = ceil((double)(value - low) / (double)steps * 10.0);
            percentValue = max(min(percentValue, 10), 1);
        }

        if (steps != 16)
            addDisplayBorders();

        uint8_t offset = (steps == 16) ? 0 : 3;
        if (low < 0)
            bitSet(displayData, percentValue - 1 + offset);
        else
            for (uint8_t i = 0; i < percentValue; i++)
                bitSet(displayData, i + offset);
    }

    void writeoutDisplayBuffer(uint32_t *uiData, uint32_t *uiFlashData)
    {
        *uiData &= (uint32_t)0xFFFF0000;
        *uiFlashData &= (uint32_t)0xFFFF0000;
        *uiData |= displayData;
        *uiFlashData |= flashData;
    }

    bool didClose()
    {
        if (_didClose)
        {
            _didClose = false;
            return true;
        }
        return false;
    }
};

#endif