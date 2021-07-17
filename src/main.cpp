/*
 - 
*/

#define LOGGING false
#define GRAPHING false
#define SHOWMEM false

#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "sequencer.h"
#include "controls.h"
#include "dac.h"
#include "memory.h"
#include "ImTimer.h"
#include "glide.h"
#include "display.h"
#include "knob.h"
#include "patternLoad.h"
#include "MemoryFree.h"

#pragma region FUNCTION HEADERS
/* --------------- FUNCTION HEADERS -----------------
*/

void setupSequencer();
void setupKnobs();
void handleFunctionButtons();
void handleEncoderButtons();
void handleLeftRotaryEncoder();
void handleMiddleRotaryEncoder();
void handleRightRotaryEncoder();
void handlePianoKeys();
void updateControls();
void showFreeMemory(uint8_t i);
void cvOut(uint8_t channel, uint16_t v);
#pragma endregion

#pragma region GLOBAL VARS
/* ---------------- GLOBAL VARS ------------------
*/

MP4822 dac;
Sequencer *seq;
void bpmClockCallback() { seq->bpmClockTick(); }
void gateTimerCallback() { seq->closeGate(); }
void clockLedTimerCallback() { seq->clockLedOff(); }
void interruptCallback() { seq->externalClockTrigger(); }

#pragma endregion

#pragma region SETUP
// ---------------- SETUP ----------------

void setup()
{
    Serial.begin(115200);
    //String mytest = String(F("Loading...")); // required!!!!
#if (LOGGING)
    Serial.println(mytest);
#endif
    showFreeMemory(1);
    setupSequencer();
    setupLeds();
    setupIO();
    setupKnobs();
    attachInterrupt(digitalPinToInterrupt(CLK_IN), interruptCallback, RISING);
    bpmClock.start(looping, 60.0 / 120 * 1000, bpmClockCallback);
    showFreeMemory(7);
}

void cvOut(uint8_t channel, uint16_t v) { dac.DAC_set(channel, v); }

void showFreeMemory(uint8_t i = 99)
{
#if (SHOWMEM)
    Serial.print(F("freemem["));
    Serial.print(i);
    Serial.print(F("]="));
    Serial.println(freeMemory());
#endif
}

void setupSequencer()
{
    bpmClock.setTickHandler(bpmClockCallback);
    gateTimer.setTickHandler(gateTimerCallback);
    clockLedTimer.setTickHandler(clockLedTimerCallback);
    seq = new Sequencer(&bpmClock, &gateTimer, &clockLedTimer);
    seq->setBpmMilliseconds(140);
}

void setupKnobs()
{
    knob[0] = new Knob(0, encoderButtons, KNOB1_A, KNOB1_B);
    knob[0]->setRange(ledOFF, 0, 20, MAXTEMPO / TEMPODIV);
    knob[0]->setRange(ledOFF, 1, 1, 50); // brightness
    knob[0]->setRange(ledOFF, 2, 1, 25); // gate duration % of note
    knob[0]->setRange(ledON, 0, 1, MAXTEMPO / TEMPODIV);
    knob[0]->setRange(ledON, 1, 1, 50); // brightness
    knob[0]->setRange(ledON, 2, 1, 25); // gate duration % of note
    knob[0]->addModes(new KnobFunction[6]{
        TempoAdjust, StepSelect, GateTime,
        TempoAdjust, StepSelect, GateTime});
    knob[0]->setMode(0);
    knob[0]->setValue(120 / 10); // bpmMilliseconds

    knob[1] = new Knob(1, encoderButtons, KNOB2_A, KNOB2_B);
    knob[1]->setRange(ledOFF, 0, 1, 5);    // play mode
    knob[1]->setRange(ledOFF, 1, 0, 24);   // glide time
    knob[1]->setRange(ledOFF, 2, -24, 24); // pitch
    knob[1]->setRange(ledON, 0, 1, 4);     // play mode
    knob[1]->setRange(ledON, 1, 0, 24);    // glide time
    knob[1]->setRange(ledON, 2, 1, 12);    // pitch
    knob[1]->addModes(new KnobFunction[6]{
        PlayMode, GlideTime, Pitch,
        PlayMode, GlideTime, Pitch});
    knob[1]->setMode(0);

    knob[2] = new Knob(2, encoderButtons, KNOB3_A, KNOB3_B);
    knob[2]->setRange(ledOFF, 0, 1, PATTERN_STEP_MAX); // pattern length
    knob[2]->setRange(ledOFF, 1, 0, 3);                // glide shape/curve
    knob[2]->setRange(ledOFF, 2, 1, 8);                // octave
    knob[2]->setRange(ledON, 0, 1, PATTERN_STEP_MAX);  // pattern length
    knob[2]->setRange(ledON, 1, 0, 3);                 // glide shape/curve
    knob[2]->setRange(ledON, 2, 1, 8);                 // octave
    knob[2]->addModes(new KnobFunction[9]{
        NumSteps, GlideShape, Octave,
        NumSteps, GlideShape, Octave});
    knob[2]->setMode(0);
    knob[2]->setValue(16);
}

#pragma endregion

#pragma region MAIN LOOP
/* ---------------- LOOP ----------------
*/
void loop()
{
    bpmClock.update();
    gateTimer.update();
    clockLedTimer.update();
    cvOut(0, seq->getPitchCV());
    updateControls();
    updateDisplay();
}

#pragma endregion

#pragma region CONTROLS HANDLING
// ---------------- CONTROLS HANDLING  ----------------

void updateControls()
{
    encoderButtons.update();
    handleEncoderButtons();

    handleLeftRotaryEncoder();
    handleMiddleRotaryEncoder();
    handleRightRotaryEncoder();

    funcButtons.update();
    handleFunctionButtons();

    pianoBlack.update();
    pianoWhite.update();
    handlePianoKeys();
}

void handleFunctionButtons()
{
        
    // FUNCTION BUTTONS
    if (funcButtons.onPress(SHIFT))
    {
        /*
        Serial.print("shift-pressed");
        Serial.print("\tshift: ");
        Serial.print(FUNCTIONS::SHIFT);
        Serial.print("\tplay: ");
        Serial.println(FUNCTIONS::PLAY);
        */
        LedState newState = (getLedState(ledSHIFT) == ledON) ? ledOFF : ledON;
        setLedState(ledSHIFT, newState);
    }

    if (funcButtons.onPress(PLAY))
    {
        /*
        Serial.print("play-pressed");
        Serial.print("\tshift: ");
        Serial.print(FUNCTIONS::SHIFT);
        Serial.print("\tplay: ");
        Serial.println(FUNCTIONS::PLAY);
        Serial.println("play");
        */
        if (getLedState(ledSHIFT) == ledON)
        {
            bool recordState = (getLedState(ledPLAY) != ledFLASH); // Toggle recording
            seq->setRecording(recordState);
            if (recordState)
            {
                knob[0]->setMode(KnobFunction::StepSelect);
                knob[1]->setMode(KnobFunction::PlayMode);
                knob[2]->setMode(KnobFunction::Octave);
            }
        }
        else
        {
            setLedState(ledPLAY, getLedState(ledPLAY)); // Toggle Play
            if (ioState(ledPLAY) == ledON)
                seq->pause();
            else
                seq->play();
        }
    }

    if (funcButtons.onPress(ENTER))
    {
        if (seq->isStepEditing())
            seq->patternInsertRest();
        else
            showFreeMemory(99);
    }

    if (funcButtons.onPress(SAVE))
    {
        /*
        if (currentMode == MODE_SEQUENCER) { // SAVE : SELECT BANK
          currentMode == MODE_BANKSELECT;
          setValuePicker(1, 1, 4, 0);
          setLedState(ledENTER, ledFLASH);
        }
        */
        savePattern(0, 0);
    }

    if (funcButtons.onPress(LOAD))
    {
        loadPattern(0, 0);
    }
}

void handleEncoderButtons()
{
    for (uint8_t i = 0; i < 3; i++)
    {
        knob[i]->update();
        if (encoderButtons.onPress(i))
            knob[i]->nextMode();
    }
}

void handlePianoKeys()
{
    for (uint8_t i = 0; i < KBDW_BUTTONS_TOTAL; i++)
        if (pianoWhite.onPress(i))
        {
            seq->pianoKeyPressed(pitchIndexWhite[i]);
            break;
        }

    for (uint8_t i = 0; i < KBDB_BUTTONS_TOTAL; i++)
        if (pianoBlack.onPress(i))
        {
            seq->pianoKeyPressed(pitchIndexBlack[i]);
            break;
        }
}

#pragma region ROTARY ENCODERS

void handleLeftRotaryEncoder()
{
    Knob *k = knob[0];
    short value = k->value();
    short knobDirection = k->direction();
    if (k->didChange())
    {
        //log("left encoder value changed\n");
        switch (k->getMode())
        {

        case 0: // TEMPO
        {
            uint16_t newTempo;
            int16_t steps = k->getRangeMax() - k->getRangeMin();
            int16_t precisionPoint = steps / 2.0 + (k->getRangeMin() - 1);

            if (value <= precisionPoint)
                newTempo = k->getRangeMin() + ((value - k->getRangeMin()) * 25);
            else
            {
                int16_t nonLinearSteps = k->getRangeMax() - precisionPoint;
                float factor = (value - precisionPoint) / (double)nonLinearSteps;
                newTempo = value * factor * TEMPODIV;
                newTempo += k->getRangeMin() + ((value - k->getRangeMin()) * 25);
                //newTempo = max(newTempo, precisionPoint);
            }
            seq->setBpmMilliseconds(newTempo);
            setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax(), DIALOG_TIMEOUT);
            break;
        }
        case 1:
        {
            switch (getLedState(ledSHIFT))
            {
            case ledOFF: // STEP SELECT
                if (knobDirection != 0)
                {
                    short newStep = seq->selectStep(knobDirection);
                    setSequencerStep(newStep);
                }
                break;

            case ledON: // SHIFT-Brightness
                setBrightness(value * 5);
                setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax(), DIALOG_TIMEOUT);
                break;

            case ledFLASH: // not assigned
                break;
            }
            break;
        }

        case 2: // Gate Length
        {
            seq->setGateLength(4 * value);
            setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax(), DIALOG_TIMEOUT);
            break;
        }
        }
        showFreeMemory();
    }
}

void handleMiddleRotaryEncoder()
{
    Knob *k = knob[1];
    if (k->didChange())
    {
        //log("middle encoder value changed\n");
        short value = k->value();

        switch (k->getMode())
        {
        case 0: // playMode
            playMode = static_cast<PlayModes>(k->value());
            setValuePicker(value, k->getRangeMin(), k->getRangeMax());
            break;

        case 1: // glide time

            seq->setGlideTime(value / (float)knob[1]->getRangeMax());
            setValuePicker(value, k->getRangeMin(), k->getRangeMax());
            break;

        case 2: // pitch
            seq->setTranspose(k->direction());
            setValuePicker(seq->getTranspose(), k->getRangeMin(), k->getRangeMax());
            break;
        }
        showFreeMemory();
    }
}

void handleRightRotaryEncoder()
{

    if (knob[2]->didChange())
    {
        //log("right encoder value changed\n");
        short value = knob[2]->value();
        switch (knob[2]->getMode())
        {

        case 0: // pattern length
            seq->setPatternLength(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax(), DIALOG_TIMEOUT);
            break;

        case 1: // glide shape
            seq->setCurveShape(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax(), DIALOG_TIMEOUT);
            break;

        case 2: // octave
            seq->setOctave(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax(), DIALOG_TIMEOUT);
            break;
        }
        showFreeMemory();
    }
}

#pragma endregion

#pragma endregion