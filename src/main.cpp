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
#include "logger.h"

#if (SHOWMEM)
#include "MemoryFree.h"
#endif

#pragma region FUNCTION HEADERS
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
void updateLoading();
void updateSaving();
void updateKnobs();
void updatePatternStorage();

#pragma endregion

#pragma region GLOBAL VARS

MP4822 dac;
Sequencer *seq;
StorageAction storageAction = StorageAction::LOAD_PATTERN;

void bpmClockCallback() { seq->bpmClockTick(); }
void gateTimerCallback() { seq->closeGate(); }
void clockLedTimerCallback() { seq->clockLedOff(); }
void interruptCallback() { seq->externalClockTrigger(); }

#pragma endregion

#pragma region SETUP

void setup()
{
    Serial.begin(115200);
#if (LOGGING)
    Serial.println(F("loading..."));
#endif
    showFreeMemory(1);
    setupSequencer();
    setupDisplay();
    setupIO();
    setupKnobs();
    attachInterrupt(digitalPinToInterrupt(CLK_IN), interruptCallback, RISING);
    bpmClock.start(looping, 60.0 / 120 * 1000, bpmClockCallback);
    showFreeMemory(7);
}

void cvOut(uint8_t channel, int16_t v) { dac.DAC_set(channel, v); }

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

void loop()
{
    bpmClock.update();
    gateTimer.update();
    clockLedTimer.update();
    cvOut(0, seq->getPitchCV());

    switch (uiState)
    {
    case UIState::SEQUENCER:
        updateControls();
    case UIState::SA_BANK_SELECT:
    case UIState::SA_PATTERN_SELECT:
    case UIState::SA_ACTION_COMPLETE:
        updatePatternStorage();
    }

    updateDisplay();
}

#pragma endregion

#pragma region STATE LOADING / SAVING

void selectBank(Knob *k, UIState nextState)
{
    k->update();

    if (uiStateChanged())
    {
        Serial.println(F("select bank"));
        k->setValue(memBank);
        setLedState(ledENTER, LedState::ledFLASH);
        for (byte i = 16; i < 25; i++)
            setLedState(i, (i < 22) ? ledOFF : ledFLASH);
        setLedState(16, ledFLASH);
        setValuePicker(memBank, 0, 3, false);
    }

    if (k->didChange())
    {
        memBank = constrain(memBank + k->direction(), 0, BANK_MAX - 1);
        Serial.print(F("bank: "));
        Serial.println(memBank);
        setValuePicker(memBank, 0, BANK_MAX - 1, false);
    }

    funcButtons.update();
    if (funcButtons.onPress(FUNCTIONS::ENTER))
        uiState = nextState;
}

void selectPattern(Knob *k, UIState nextState)
{
    k->update();

    if (uiStateChanged())
    {
        Serial.println(F("select pattern"));
        k->setValue(memPattern);
        setLedState(19, ledFLASH);
        setValuePicker(memPattern, 0, PATTERN_MAX - 1, false);
    }

    if (k->didChange())
    {
        memPattern = constrain(memPattern + k->direction(), 0, PATTERN_MAX - 1);
        Serial.print(F("pattern: "));
        Serial.println(memPattern);
        setValuePicker(memPattern, 0, PATTERN_MAX - 1, false);
    }

    funcButtons.update();
    if (funcButtons.onPress(FUNCTIONS::ENTER))
        uiState = nextState;
}

void finishedStorageAction()
{
    setValuePicker(9, 0, 9, true, 500);
    setLedState(ledENTER, LedState::ledOFF);
    uiState = UIState::SEQUENCER;
    Serial.println(F("load/save complete"));
}

void updatePatternStorage()
{
    switch (uiState)
    {
    case UIState::SA_BANK_SELECT:
        selectBank(knob[2], UIState::SA_PATTERN_SELECT);
        break;

    case UIState::SA_PATTERN_SELECT:
        selectPattern(knob[2], UIState::SA_ACTION_COMPLETE);        
        break;

    case UIState::SA_ACTION_COMPLETE:
        if (storageAction == StorageAction::LOAD_PATTERN)
            loadPattern(memBank, memPattern);
        else
            savePattern(memBank, memPattern);
        finishedStorageAction();
        break;
    default:
        break;
    }
}
/*
void updateLoading()
{
    switch (uiState)
    {
    case UIState::LOADING_BANK_SELECT:
        selectBank(knob[2], UIState::LOADING_PATTERN_SELECT);

    case UIState::LOADING_PATTERN_SELECT:
        selectPattern(knob[2], UIState::LOADING_COMPLETE);

    case UIState::LOADING_COMPLETE:        
        loadPattern(memBank, memPattern);
        finishedStorageAction();
    default:
        break;
    }
}

void updateSaving()
{
    switch (uiState)
    {
    case UIState::SAVING_BANK_SELECT:
        selectBank(knob[2], UIState::SAVING_PATTERN_SELECT);

    case UIState::SAVING_PATTERN_SELECT:
        selectPattern(knob[2], UIState::SAVING_COMPLETE);

    case UIState::SAVING_COMPLETE:
        savePattern(memBank, memPattern);
        finishedLoadSave();

    default:
        break;
    }
}
*/

void refreshKnobLeds()
{
    knob[0]->setLED();
    knob[1]->setLED();
    knob[2]->setLED();
}

#pragma endregion

#pragma region CONTROLS HANDLING

void updateControls()
{
    if (uiStateChanged())
        refreshKnobLeds();

    encoderButtons.update();
    handleEncoderButtons();

    funcButtons.update();
    handleFunctionButtons();

    updateKnobs();
    handleLeftRotaryEncoder();
    handleMiddleRotaryEncoder();
    handleRightRotaryEncoder();

    pianoBlack.update();
    pianoWhite.update();
    handlePianoKeys();
}

void handleFunctionButtons()
{
    // FUNCTION BUTTONS
    if (funcButtons.onPress(SHIFT))
    {
#if (DEBUG)
        Serial.println(F("shift-pressed"));
#endif

        LedState newState = (getLedState(ledSHIFT) == ledON) ? ledOFF : ledON;
        setLedState(ledSHIFT, newState);
    }

    if (funcButtons.onPress(PLAY))
    {
#if (DEBUG)
        Serial.print(F("play-pressed"));
#endif

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
        Serial.println(F("Enter pressed"));
        if (seq->isStepEditing())
            seq->patternInsertRest();
        else
            showFreeMemory(99);
    }

    if (funcButtons.onPress(SAVE))
    {
        Serial.println(F("SAVE pressed"));
        storageAction = StorageAction::SAVE_PATTERN;
        uiState = UIState::SA_BANK_SELECT;        
    }

    if (funcButtons.onPress(LOAD))
    {
        Serial.println(F("LOAD pressed"));
        storageAction = StorageAction::LOAD_PATTERN;
        uiState = UIState::SA_BANK_SELECT;
    }
}

void updateKnobs()
{
    for (uint8_t i = 0; i < 3; i++)
        knob[i]->update();
}

void handleEncoderButtons()
{
    for (uint8_t i = 0; i < 3; i++)
    {
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
            //Serial.println(F("tempo"));
            setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax());
            break;
        }
        case 1:
        {
            switch (getLedState(ledSHIFT))
            {
            case ledOFF: // STEP SELECT
                //Serial.println(F("step select"));
                if (knobDirection != 0)
                {
                    short newStep = seq->selectStep(knobDirection);
                    setSequencerStep(newStep);
                }
                break;

            case ledON: // SHIFT-Brightness
                //Serial.println(F("brightness"));
                setBrightness(value * 5);
                setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax());
                break;

            case ledFLASH: // not assigned
                break;
            }
            break;
        }

        case 2: // Gate Length
        {
            //Serial.println(F("g-length"));
            seq->setGateLength(4 * value);
            setValuePicker(value, knob[0]->getRangeMin(), knob[0]->getRangeMax());
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
            //Serial.println(F("play mode"));
            playMode = static_cast<PlayModes>(k->value());
            setValuePicker(value, k->getRangeMin(), k->getRangeMax());
            break;

        case 1: // glide time
            //Serial.println(F("g-time"));
            seq->setGlideTime(value / (float)knob[1]->getRangeMax());
            setValuePicker(value, k->getRangeMin(), k->getRangeMax());
            break;

        case 2: // pitch
            //Serial.println(F("pitch"));
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
            //Serial.println(F("p-length"));
            seq->setPatternLength(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax());
            break;

        case 1: // glide shape
            //Serial.println(F("g-shape"));
            seq->setCurveShape(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax());
            break;

        case 2: // octave
            //Serial.println(F("octave"));
            seq->setOctave(value);
            setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax());
            break;
        }
        showFreeMemory();
    }
}

#pragma endregion

#pragma endregion