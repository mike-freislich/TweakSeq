#define LOGGING false
#define SHOWMEM true

#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "sequencer.h"
#include "controls.h"
#include "dac.h"
#if (SHOWMEM)
#include "memory.h"
#endif
#include "glide.h"
#include "ShiftRegisterPWM.h"
#include "knob.h"
#include "SimpleKnob.h"
#include "seqState.h"

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
Sequencer seq;
ShiftRegisterPWM sr;
StorageAction storageAction = StorageAction::LOAD_PATTERN;

void interruptCallback() { seq.externalClockTrigger(); }
void cvOut(uint8_t channel, int16_t v) { dac.DAC_set(channel, v); }

#pragma endregion

#pragma region SETUP

void setup()
{
#if (LOGGING) || (SHOWMEM)
    Serial.begin(57600);
#endif
#if (LOGGING)
    Serial.println(F("loading..."));
#endif
    showFreeMemory(1);
    setupIO();
    setupKnobs();

    sr.interrupt(ShiftRegisterPWM::UpdateFrequency::Slow);
    attachInterrupt(digitalPinToInterrupt(CLK_IN), interruptCallback, RISING);
    seq.setBpm(140);
    loadPattern(0, 0);
    seq.setPatternLength(pattern.length);
    showFreeMemory(7);
}

void setupKnobs()
{
    knob[0] = new Knob(0, encoderButtons, KNOB1_A, KNOB1_B);
    knob[0]->setRange(ledOFF, 0, 0, MAXTEMPO / TEMPODIV);
    knob[0]->setRange(ledOFF, 1, 1, 50); // brightness
    knob[0]->setRange(ledOFF, 2, 0, 24); // gate duration % of note
    knob[0]->setRange(ledON, 0, 20, MAXTEMPO / TEMPODIV);
    knob[0]->setRange(ledON, 1, 1, 50);   // brightness
    knob[0]->setRange(ledON, 2, -20, 20); // shuffle -10:hard shuffle | 0:no shuffle | +10: hard reverse shuffle
    knob[0]->addModes(new KnobFunction[6]{TempoAdjust, StepSelect, GateTime, TempoAdjust, StepSelect, GateTime});
    knob[0]->setMode(0);
    knob[0]->setValue(120 / 10); // bpmMilliseconds

    knob[1] = new Knob(1, encoderButtons, KNOB2_A, KNOB2_B);
    knob[1]->setRange(ledOFF, 0, 1, 5);    // play mode
    knob[1]->setRange(ledOFF, 1, 0, 24);   // glide time
    knob[1]->setRange(ledOFF, 2, -24, 24); // pitch
    knob[1]->setRange(ledON, 0, 1, 4);     // play mode
    knob[1]->setRange(ledON, 1, 0, 24);    // glide time
    knob[1]->setRange(ledON, 2, -24, 24);  // pitch
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
    seq.update();

    cvOut(0, seq.getPitchCV());

    switch (uiState)
    {
    case UIState::SEQUENCER:
        updateControls();
    case UIState::ACTION_BANK_SELECT:
    case UIState::ACTION_PATTERN_SELECT:
    case UIState::ACTION_COMPLETE:
        updatePatternStorage();
    }
}

#pragma endregion

#pragma region STATE LOADING / SAVING

void selectBank(Knob *k, UIState nextState)
{
    k->update();

    if (uiStateChanged())
    {
#if (LOGGING)
        Serial.println(F("select bank"));
#endif
        k->setValue(memBank);
        sr.set(ledENTER, LedState::ledFLASH);
        for (byte i = 16; i < 25; i++)
            sr.set(i, (i < 22) ? ledOFF : ledON);
        sr.set(16, LedState::ledFLASH);
        seq.setValuePicker(memBank, 0, 3, false);
    }

    if (k->didChange())
    {
        memBank = constrain(memBank + k->direction(), 0, BANK_MAX - 1);
#if (LOGGING)
        Serial.print(F("bank: "));
        Serial.println(memBank);
#endif
        seq.setValuePicker(memBank, 0, BANK_MAX - 1, false);
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
#if (LOGGING)
        Serial.println(F("select pattern"));
#endif
        k->setValue(memPattern);
        sr.set(19, LedState::ledFLASH);
        seq.setValuePicker(memPattern, 0, PATTERN_MAX - 1, false);
    }

    if (k->didChange())
    {
        memPattern = constrain(memPattern + k->direction(), 0, PATTERN_MAX - 1);
#if (LOGGING)
        Serial.print(F("pattern: "));
        Serial.println(memPattern);
#endif
        seq.setValuePicker(memPattern, 0, PATTERN_MAX - 1, false);
    }

    funcButtons.update();
    if (funcButtons.onPress(FUNCTIONS::ENTER))
        uiState = nextState;
}

void finishedStorageAction()
{
    seq.setValuePicker(9, 0, 9, true, 500);
    seq.setPatternLength(pattern.length);
    seq.setShuffle(pattern.shuffle);
    sr.set(ledENTER, LedState::ledOFF);
    uiState = UIState::SEQUENCER;
#if (LOGGING)
    Serial.println(F("load/save complete"));
#endif
}

void updatePatternStorage()
{
    switch (uiState)
    {
    case UIState::ACTION_BANK_SELECT:
        selectBank(knob[2], UIState::ACTION_PATTERN_SELECT);
        break;

    case UIState::ACTION_PATTERN_SELECT:
        selectPattern(knob[2], UIState::ACTION_COMPLETE);
        break;

    case UIState::ACTION_COMPLETE:
        if (storageAction == StorageAction::LOAD_PATTERN)
            loadPattern(memBank, memPattern);
        else
            savePattern(memBank, memPattern);
        finishedStorageAction();

    default:
        break;
    }
}

void showKnobSelectorLeds()
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
        showKnobSelectorLeds();

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
#if (LOGGING)
        Serial.println(F("shift-pressed"));
#endif

        LedState newState = (sr.get(ledSHIFT) == ledON) ? ledOFF : ledON;
        sr.set(ledSHIFT, newState);
    }

    if (funcButtons.onPress(PLAY))
    {
#if (LOGGING)
        Serial.println(F("play-pressed"));
#endif

        if (sr.get(ledSHIFT) == LedState::ledON)
        {
            bool recordState = (sr.get(ledPLAY) != ledFLASH); // Toggle recording
            seq.setRecording(recordState);
            if (recordState)
            {
                knob[0]->setMode(KnobFunction::StepSelect);
                knob[1]->setMode(KnobFunction::PlayMode);
                knob[2]->setMode(KnobFunction::Octave);
            }
        }
        else
        {
            sr.toggle(ledPLAY);
            if (sr.get(ledPLAY) == ledOFF)
                seq.pause();
            else
                seq.play();
        }
    }

    if (seq.isStepEditing())
    {
        if (funcButtons.onPressAfter(ENTER, 1000))
        {
            seq.dimStep();
            seq.patternInsertTie();
        }

        if (funcButtons.onReleaseBefore(ENTER, 500))
        {
            seq.flashStep();
            seq.patternInsertRest();
        }
    }

    if (funcButtons.onPress(ENTER))
    {
#if (LOGGING)
        Serial.println(F("Enter pressed"));
#endif
        showFreeMemory(99);
    }

    if (funcButtons.onPress(SAVE))
    {
#if (LOGGING)
        Serial.println(F("SAVE pressed"));
#endif
        storageAction = StorageAction::SAVE_PATTERN;
        uiState = UIState::ACTION_BANK_SELECT;
    }

    if (funcButtons.onPress(LOAD))
    {
#if (LOGGING)
        Serial.println(F("LOAD pressed"));
#endif
        storageAction = StorageAction::LOAD_PATTERN;
        uiState = UIState::ACTION_BANK_SELECT;
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
            seq.pianoKeyPressed(pitchIndexWhite[i]);
            break;
        }

    for (uint8_t i = 0; i < KBDB_BUTTONS_TOTAL; i++)
        if (pianoBlack.onPress(i))
        {
            seq.pianoKeyPressed(pitchIndexBlack[i]);
            break;
        }
}

#pragma endregion

#pragma region ROTARY ENCODERS

void handleLeftRotaryEncoder()
{
    Knob *k = knob[0];
    short value = k->value();
    short knobDirection = k->direction();
    if (k->didChange())
    {
        switch (k->getMode())
        {
        case 0: // TEMPO
        {
            uint16_t newTempo;
            int16_t steps = k->getRangeMax() - k->getRangeMin();
            int16_t precisionPoint = steps / 2.0 + (k->getRangeMin() - 1);

#if (LOGGING)
            char buffer[100];
            sprintf(buffer, "value: %d\tsteps: %d\tx-over: %d\t", value, steps, precisionPoint);
            Serial.print(buffer);
#endif
            if (value <= precisionPoint)
                newTempo = k->getRangeMin() + ((value - k->getRangeMin()) * 10) + 20;
            else
            {
#if (LOGGING)
                Serial.print(" XO ");
#endif
                int16_t nonLinearSteps = k->getRangeMax() - precisionPoint;
                float factor = (value - precisionPoint) / (double)nonLinearSteps;
                newTempo = value * factor * TEMPODIV;
                newTempo += k->getRangeMin() + ((value - k->getRangeMin()) * 10);
            }
            seq.setBpm(newTempo);
#if (LOGGING)
            Serial.print(F("bpm: "));
            Serial.println(newTempo);
#endif
            seq.setValuePicker(value, k->getRangeMin(), k->getRangeMax());

            break;
        }
        case 1:
            switch (sr.get(ledSHIFT))
            {
            case ledOFF: // STEP SELECT
                if (knobDirection != 0)
                {
                    short newStep = seq.selectStep(knobDirection);
                    seq.setStep(newStep);
                    seq.displayStep();
                }
                break;

            case ledON: // SHIFT-Brightness
                sr.setPulseWidth(value * 5);
                seq.setValuePicker(value, k->getRangeMin(), k->getRangeMax());
                break;

            default:
                break;
            }
            break;

        case 2: // Gate Length
            switch (sr.get(ledSHIFT))
            {
            case LedState::ledOFF:
                seq.setGateLength(4 * value + 1);
                seq.setValuePicker(value, k->getRangeMin(), k->getRangeMax());
                break;
            case LedState::ledON: // Shuffle
            {
                int8_t shuffleDisplayValue = (seq.changeShuffle(k->direction()) - 50) / 2;
                k->setValue(shuffleDisplayValue);
                seq.setValuePicker(shuffleDisplayValue, k->getRangeMin(), k->getRangeMax());
                break;
            }
            default:
                break;
            }

            break;
        }
        showFreeMemory();
    }
}

void handleMiddleRotaryEncoder()
{
    Knob *k = knob[1];
    if (k->didChange())
    {
        short value = k->value();
        switch (k->getMode())
        {
        case 0: // playMode
            playMode = static_cast<PlayModes>(k->value());
            seq.setValuePicker(value, k->getRangeMin(), k->getRangeMax());
            break;

        case 1: // glide time
            Serial.println(value / (float)k->getRangeMax());
            seq.setGlideTime(value / (float)k->getRangeMax());
            seq.setValuePicker(value, k->getRangeMin(), k->getRangeMax());
            break;

        case 2: // pitch
            seq.setTranspose(k->direction());
            int8_t newTranspose = seq.getTranspose();
            k->setValue(newTranspose);
            seq.setValuePicker(newTranspose, k->getRangeMin(), k->getRangeMax());
            break;
        }
        showFreeMemory();
    }
}

void handleRightRotaryEncoder()
{

    if (knob[2]->didChange())
    {
        short value = knob[2]->value();
        switch (knob[2]->getMode())
        {

        case 0: // pattern length
            seq.setPatternLength(value);
            seq.setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax());
            break;

        case 1: // glide shape
            seq.setCurveShape((Glide::CurveType)value);
            seq.setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax());
            break;

        case 2: // octave
            seq.setOctave(value);
            seq.setValuePicker(value, knob[2]->getRangeMin(), knob[2]->getRangeMax());
            break;
        }
        showFreeMemory();
    }
}

#pragma endregion
