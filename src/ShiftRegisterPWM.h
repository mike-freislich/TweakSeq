#ifndef ShiftRegisterPWM_h
#define ShiftRegisterPWM_h

#include <Arduino.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "SimpleTimer.h"

#define DATA_PIN 4 // Shift Register - pin 14
#define LATCH_PIN 5
#define CLOCK_PIN 6
#define OUTPUT_ENABLE_PIN 3

#define ShiftRegisterPWM_DATA_PORT PORTD
#define ShiftRegisterPWM_DATA_MASK 0b00010000
#define ShiftRegisterPWM_LATCH_PORT PORTD
#define ShiftRegisterPWM_LATCH_MASK 0b00100000
#define ShiftRegisterPWM_CLOCK_PORT PORTD
#define ShiftRegisterPWM_CLOCK_MASK 0b01000000

#define ShiftRegisterPWM_setDataPin() ShiftRegisterPWM_DATA_PORT |= ShiftRegisterPWM_DATA_MASK;
#define ShiftRegisterPWM_clearDataPin() ShiftRegisterPWM_DATA_PORT &= ~ShiftRegisterPWM_DATA_MASK;
#define ShiftRegisterPWM_toggleClockPinTwice()                  \
    ShiftRegisterPWM_CLOCK_PORT ^= ShiftRegisterPWM_CLOCK_MASK; \
    ShiftRegisterPWM_CLOCK_PORT ^= ShiftRegisterPWM_CLOCK_MASK
#define ShiftRegisterPWM_toggleLatchPinTwice()                  \
    ShiftRegisterPWM_LATCH_PORT ^= ShiftRegisterPWM_LATCH_MASK; \
    ShiftRegisterPWM_LATCH_PORT ^= ShiftRegisterPWM_LATCH_MASK

#ifndef ShiftRegisterPWM_IGNORE_PINS
#define ShiftRegisterPWM_IGNORE_PINS \
    {                                \
        29, 30                       \
    }
#endif

const uint8_t ignorePins[] = ShiftRegisterPWM_IGNORE_PINS; //ShiftRegisterPWM_IGNORE_PINS;
const uint8_t resolution = 255;                            // number of brightness increments
volatile uint8_t dutyCounter = 0;
volatile uint8_t isrCounter = 0;
uint32_t ioData = (uint32_t)0x00000000;
uint32_t ioFlashData = (uint32_t)0x00000000;

uint8_t speed = resolution;
uint32_t dutyCycleMask;

class ShiftRegisterPWM
{
private:
    SimpleTimer flashTimer = SimpleTimer(300);
    bool flashState = true;

    inline void shiftOut(uint8_t data) const
    {
        // unrolled for loop
        // bit 0 (LSB)
        if (data & 0B10000000)
        {
            ShiftRegisterPWM_setDataPin();
        }
        else
        {
            ShiftRegisterPWM_clearDataPin();
        }
        ShiftRegisterPWM_toggleClockPinTwice();

        // bit 1
        if (data & 0B01000000)
        {
            ShiftRegisterPWM_setDataPin();
        }
        else
        {
            ShiftRegisterPWM_clearDataPin();
        }
        ShiftRegisterPWM_toggleClockPinTwice();

        // bit 2
        if (data & 0B00100000)
        {
            ShiftRegisterPWM_setDataPin();
        }
        else
        {
            ShiftRegisterPWM_clearDataPin();
        }
        ShiftRegisterPWM_toggleClockPinTwice();

        // bit 3
        if (data & 0B00010000)
        {
            ShiftRegisterPWM_setDataPin();
        }
        else
        {
            ShiftRegisterPWM_clearDataPin();
        }
        ShiftRegisterPWM_toggleClockPinTwice();

        // bit 4
        if (data & 0B00001000)
        {
            ShiftRegisterPWM_setDataPin();
        }
        else
        {
            ShiftRegisterPWM_clearDataPin();
        }
        ShiftRegisterPWM_toggleClockPinTwice();

        // bit 5
        if (data & 0B00000100)
        {
            ShiftRegisterPWM_setDataPin();
        }
        else
        {
            ShiftRegisterPWM_clearDataPin();
        }
        ShiftRegisterPWM_toggleClockPinTwice();

        // bit 6
        if (data & 0B00000010)
        {
            ShiftRegisterPWM_setDataPin();
        }
        else
        {
            ShiftRegisterPWM_clearDataPin();
        }
        ShiftRegisterPWM_toggleClockPinTwice();

        // bit 7
        if (data & 0B00000001)
        {
            ShiftRegisterPWM_setDataPin();
        }
        else
        {
            ShiftRegisterPWM_clearDataPin();
        }
        ShiftRegisterPWM_toggleClockPinTwice();
    };

public:
    enum UpdateFrequency
    {
        VerySlow, // 6,400 Hz interrupt
        Slow,     // 12,800 Hz interrupt
        Medium,   // 25,600 Hz interrupt
        Fast,     // 35,714 Hz interrupt
        SuperFast // 51,281 Hz interrupt
    };

    static ShiftRegisterPWM *singleton; // used inside the ISR

    /**
    * Constructor for a new ShiftRegisterPWM object. 
    * An object is equivalent to one shift register or multiple, serially connected shift registers.
    * @param dataPin the Data Pin of the Shift Register
    * @param latchPin the Latch Pin of the Shift Register
    * @param clockPin the Clock Pin of the Shift Register
    */
    ShiftRegisterPWM()
    {
        ShiftRegisterPWM::singleton = this; // make this object accessible for timer interrupts
        pinMode(OUTPUT_ENABLE_PIN, OUTPUT);
        digitalWrite(OUTPUT_ENABLE_PIN, 0);
        pinMode(DATA_PIN, OUTPUT);
        pinMode(CLOCK_PIN, OUTPUT);
        pinMode(LATCH_PIN, OUTPUT);

        // set up the duty-cycle mask to ignore pins
        dutyCycleMask = 0;
        for (uint8_t x = 0; x < sizeof(ignorePins); x++)
            bitSet(dutyCycleMask, ignorePins[x]);
    }

    void setPulseWidth(uint8_t newSpeed)
    {
        speed = map(newSpeed, 0, 255, 0, resolution);
    }

    void flash()
    {
        if (flashTimer.done())
        {
            flashTimer.cycle();
            flashState = !flashState;
            /*
            for (uint8_t i = 0; i < 32; i++)
            {
                if (bitRead(ioFlashData, i))
                    bitSet(ioData, flashState);
            }
            */
            ioData ^= ioFlashData;
        }
    }

    /**
     * sets or clears the ShiftRegister pin
     * @param pin The pin on the shift register whose value will be set. 0-7 on SR1, 8-15 on SR2, .... 31 (4 SRs)
     * @param value Should be set to 0 or 1 for now ... TODO: later will add flashing, or even better PULSING!
     */
    void set(uint8_t pin, uint8_t value)
    {
        switch (value)
        {
        case 1:
            bitSet(ioData, pin);
            //bitClear(ioFlashData, pin);
            break;
        case 2:
            //bitSet(ioData, pin);
            bitSet(ioFlashData, pin);
            break;
        default:
            bitClear(ioData, pin);
            bitClear(ioFlashData, pin);
        }
    }

    void toggle(uint8_t pin)
    {
        set(pin, !get(pin));
    }

    uint8_t get(uint8_t pin)
    {
        return bitRead(ioFlashData, pin) == 1 ? 2 : bitRead(ioData, pin);
    }

    void setData(uint32_t data)
    {
        ioData = data;
        ioFlashData = 0x00000000;
    }

    /** 
    * Updates the shift register outputs. This function should be called as frequently as possible, usually within an ISR to guarantee a fixed update frequency.
    * For manual operation it is important to ensure that the function is called with a constant frequency that is suitable for the application. Commonly that is around 50 Hz * resolution for LEDs.
    */
    void update()
    {
        uint32_t dataForWrite = ioData;

        if (dutyCounter > speed)           // if not dutyCycle
            dataForWrite &= dutyCycleMask; // set the ShiftRegister pins to PWM off duty cycle

        shiftOut(dataForWrite >> 24);
        shiftOut(dataForWrite >> 16);
        shiftOut(dataForWrite >> 8);
        shiftOut(dataForWrite);
        ShiftRegisterPWM_toggleLatchPinTwice();

        // update the pulseCounter
        dutyCounter++;
        if (++isrCounter > resolution)
        {
            dutyCounter = 0;
            isrCounter = 0;
        }
    }

    /**
    * Calls void ShiftRegisterPWM::interrupt(UpdateFrequency updateFrequency) with the UpdateFrequency Medium 
    * Have a look at the called function for more details.
    */
    void interrupt() const
    {
        this->interrupt(ShiftRegisterPWM::UpdateFrequency::Medium);
    };

    /** 
    * Initializes and starts the timer interrupt with a given update frequency.
    * The used timer is the Arduino UNO's timer 1.
    * The function can be called multiple times with different update frequencies in order to change the update frequency at any time.
    * @param updateFrequency The update frequencies are either VerySlow @ 6,400 Hz, Slow @ 12,800 Hz, Fast @ 35,714 Hz, SuperFast @ 51,281.5 Hz, and Medium @ 25,600 Hz. 
    * The actual PWM cycle length in seconds can be calculated by (resolution / frequency).
    */
    void interrupt(UpdateFrequency updateFrequency) const
    {
        cli(); // disable interrupts

        // reset
        TCCR1A = 0; // set TCCR1A register to 0
        TCCR1B = 0; // set TCCR1B register to 0
        TCNT1 = 0;  // reset counter value

        switch (updateFrequency)
        {
        case VerySlow:             // exactly 6,400 Hz interrupt frequency
            OCR1A = 2499;          // compare match register
            TCCR1B |= (1 << CS10); // prescaler 1
            break;

        case Slow:                 // exactly 12,800 Hz interrupt frequency
            OCR1A = 1249;          // compare match register
            TCCR1B |= (1 << CS10); // prescaler 1
            break;

        case Fast:                 // aprox. 35,714 Hz interrupt frequency
            OCR1A = 55;            // compare match register
            TCCR1B |= (1 << CS11); // prescaler 8
            break;

        case SuperFast:            // approx. 51,281.5 Hz interrupt frequency
            OCR1A = 311;           // compare match register
            TCCR1B |= (1 << CS10); // prescaler 1
            break;

        case Medium: // exactly 25,600 Hz interrupt frequency
        default:
            OCR1A = 624;           // compare match register
            TCCR1B |= (1 << CS10); // prescaler 1
            break;
        }

        TCCR1B |= (1 << WGM12);  // turn on CTC mode
        TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt

        sei(); // allow interrupts
    }
};

// One static reference to the ShiftRegisterPWM that was lastly created. Used for access through timer interrupts.
ShiftRegisterPWM *ShiftRegisterPWM::singleton = {0};

//Timer 1 interrupt service routine (ISR)
ISR(TIMER1_COMPA_vect)
{          // function which will be called when an interrupt occurs at timer 1
    cli(); //cli(); // disable interrupts (in case update method takes too long)
    ShiftRegisterPWM::singleton->update();
    sei(); //sei(); // re-enable
}

#endif