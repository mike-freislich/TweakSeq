#define LOGGING true
#define GRAPHING false

#include <avr/io.h>
#include <avr/interrupt.h>
#include "logger.h"
#include "sequencer.h"
#include "controls.h"
#include "dac.h"
#include "memory.h"
#include "ImTimer.h"
#include "spline.h"
#include "glide.h"
#include "leds.h"
#include "knob.h"
#include "patternLoad.h"

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

/* ---------------- GLOBAL VARS ------------------
*/

Sequencer *seq = nullptr;
void bpmClockCallback() { seq->bpmClockTick(); }
void gateTimerCallback() { seq->closeGate(); }
void clockLedTimerCallback() { seq->clockLedOff(); }
void interruptCallback() { seq->externalClockTrigger(); }

/* ---------------- SETUP ----------------
*/

void setup()
{
    Serial.begin(115200);
    log("Loading...\n");

    setupSequencer();
    setupLeds();
    dac.init();
    setupIO();
    setupKnobs();

    attachInterrupt(digitalPinToInterrupt(CLK_IN), interruptCallback, RISING);
    bpmClock.start(looping, 60.0 / 120 * 1000, bpmClockCallback);
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
    knob[0]->setRange(ledOFF, 0, 1, MAXTEMPO / 40);
    knob[0]->setValue(120 / 10);         // bpmMilliseconds
    knob[0]->setRange(ledOFF, 1, 1, 50); // brightness
    knob[0]->setRange(ledOFF, 2, 1, 25); // gate duration % of note
    knob[0]->setRange(ledON, 0, 1, MAXTEMPO / 40);
    knob[0]->setValue(120 / 10);        // bpmMilliseconds
    knob[0]->setRange(ledON, 1, 1, 50); // brightness
    knob[0]->setRange(ledON, 2, 1, 25); // gate duration % of note
    knob[0]->setRange(ledFLASH, 0, 1, MAXTEMPO / 40);
    knob[0]->setValue(120 / 10);           // bpmMilliseconds
    knob[0]->setRange(ledFLASH, 1, 1, 50); // brightness
    knob[0]->setRange(ledFLASH, 2, 1, 25); // gate duration % of note
    knob[0]->addModes(new KnobFunction[9]{
        TempoAdjust, StepSelect, GateTime,
        TempoAdjust, StepSelect, GateTime,
        TempoAdjust, StepSelect, GateTime});
    knob[0]->setMode(0);

    knob[1] = new Knob(1, encoderButtons, KNOB2_A, KNOB2_B);
    knob[1]->setRange(ledOFF, 0, 1, 4);    // play mode
    knob[1]->setRange(ledOFF, 1, 0, 24);   // glide time
    knob[1]->setRange(ledOFF, 2, 1, 12);   // pitch
    knob[1]->setRange(ledON, 0, 1, 4);     // play mode
    knob[1]->setRange(ledON, 1, 0, 24);    // glide time
    knob[1]->setRange(ledON, 2, 1, 12);    // pitch
    knob[1]->setRange(ledFLASH, 0, 1, 4);  // play mode
    knob[1]->setRange(ledFLASH, 1, 0, 24); // glide time
    knob[1]->setRange(ledFLASH, 2, 1, 12); // pitch
    knob[1]->addModes(new KnobFunction[9]{
        PlayMode, GlideTime, Pitch,
        PlayMode, GlideTime, Pitch,
        PlayMode, GlideTime, Pitch});
    knob[1]->setMode(0);

    knob[2] = new Knob(2, encoderButtons, KNOB3_A, KNOB3_B);
    knob[2]->setRange(ledOFF, 0, 1, PATTERN_STEP_MAX);   // pattern length
    knob[2]->setRange(ledOFF, 1, 0, 3);                  // glide shape/curve
    knob[2]->setRange(ledOFF, 2, 1, 8);                  // octave
    knob[2]->setRange(ledON, 0, 1, PATTERN_STEP_MAX);    // pattern length
    knob[2]->setRange(ledON, 1, 0, 3);                   // glide shape/curve
    knob[2]->setRange(ledON, 2, 1, 8);                   // octave
    knob[2]->setRange(ledFLASH, 0, 1, PATTERN_STEP_MAX); // pattern length
    knob[2]->setRange(ledFLASH, 1, 0, 3);                // glide shape/curve
    knob[2]->setRange(ledFLASH, 2, 1, 8);                // octave
    knob[2]->addModes(new KnobFunction[9]{
        NumSteps, GlideShape, Octave,
        NumSteps, GlideShape, Octave,
        NumSteps, GlideShape, Octave});
    knob[2]->setMode(0);
    knob[2]->setValue(16);

    /*char buffer[30];
    Knob* k = knob[2];
    sprintf(buffer, "knob:%d mode:%d min:%d max:%d", k->getIndex(), k->getMode(), k->getRangeMax(), k->getRangeMin());
    log(buffer);*/
}

/* ---------------- LOOP ----------------
*/

void loop()
{
    bpmClock.update();
    gateTimer.update();
    clockLedTimer.update();
    seq->updateDAC();
    updateControls();
    updateDisplay();    
}

/* ---------------- CONTROLS HANDLING  ----------------
*/

void handleFunctionButtons()
{
    // FUNCTION BUTTONS
    if (funcButtons.onPress(SHIFT))
        nextLedState(ledSHIFT);

    if (funcButtons.onPress(PLAY))
    {
        if (getLedState(ledSHIFT) == ledON)
        {
            bool recordState = (getLedState(ledPLAY) != ledFLASH); // Toggle recording
            seq->setRecording(recordState);
            if (recordState) {
                knob[0]->setMode(1);
                knob[1]->setMode(0);
                knob[2]->setMode(2);
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
        { // STEP-EDIT : INSERT REST
            seq->patternInsertRest();
        }
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
    for (byte i = 0; i < 3; i++)
    {
        knob[i]->update();
        if (encoderButtons.onPress(i))
        {
            knob[i]->nextMode();
        }
    }
}

void handleLeftRotaryEncoder()
{
    int value = knob[0]->value();
    int knobDirection = knob[0]->direction();
    if (knob[0]->didChange())
    {
        //log("left encoder value changed\n");
        switch (knob[0]->getMode())
        {

        case 0: // Tempo
            //int scaleTempo = (ioState(ledSHIFT)) ? 1
            seq->setBpmMilliseconds(value * 20);
            setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax(), DISP_HoldValue);
            break;

        case 1:
            switch (getLedState(ledSHIFT))
            {
            case ledOFF: // Step Select
                if (knobDirection != 0)
                {
                    int newStep = seq->selectStep(knobDirection);
                    setSequencerStep(newStep);
                }
                break;

            case ledON: // SHIFT-Brightness
                setBrightness(value * 5);
                setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax(), DISP_HoldValue);
                break;

            case ledFLASH: // not assigned
                break;
            }
            break;

        case 2: // Gate Length
            seq->setGateLength(4 * value);
            setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax(), DISP_HoldValue);
            break;
        }
    }
}

void handleMiddleRotaryEncoder()
{
    if (knob[1]->didChange())
    {
        //log("middle encoder value changed\n");
        int value = knob[1]->value();

        switch (knob[1]->getMode())
        {
        case 0: // playMode
            playMode = static_cast<PlayModes>(knob[1]->value());
            setValuePicker(value, knob[1]->getRangeMin(), knob[1]->getRangeMax(), DISP_HoldValue);
            break;

        case 1: // glide time

            seq->setGlideTime(value / (float)knob[1]->getRangeMax());
            setValuePicker(value, knob[1]->getRangeMin(), knob[1]->getRangeMax(), DISP_HoldValue);
            break;

        case 2: // pitch
            //setGateTime(value);
            break;
        }
    }
}

void handleRightRotaryEncoder()
{

    if (knob[2]->didChange())
    {
        //log("right encoder value changed\n");
        int value = knob[2]->value();
        switch (knob[2]->getMode())
        {

        case 0: // pattern length
            seq->setPatternLength(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax(), DISP_HoldValue);
            break;

        case 1: // glide shape
            seq->setCurveShape(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax(), DISP_HoldValue);
            break;

        case 2: // octave
            seq->setOctave(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax(), DISP_HoldValue);
            break;
        }
    }
}

void handlePianoKeys()
{
    for (byte i = 0; i < KBDW_BUTTONS_TOTAL; i++)
    {
        if (pianoWhite.onPress(i))
        {
            seq->pianoKeyPressed(pitchIndexWhite[i]);
            break;
        }
    }

    for (byte i = 0; i < KBDB_BUTTONS_TOTAL; i++)
    {
        if (pianoBlack.onPress(i))
        {
            seq->pianoKeyPressed(pitchIndexBlack[i]);
            break;
        }
    }
}

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