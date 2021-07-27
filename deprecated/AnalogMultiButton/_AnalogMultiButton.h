/*
 * AnalogMultiButton.h
 * A library to capture multiple button presses through a single analog pin
 *
 * Copyright (c) 2016 Damien Clarke
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.  
 */
 
#ifndef ANALOG_MULTI_BUTTON_H
#define ANALOG_MULTI_BUTTON_H

#include <Arduino.h>

class AnalogMultiButton
{
  public:
    static const int MAX_BUTTONS = 20;

    // pin - the pin to read
    // total - the total number of buttons
    // values[] - an array of int analogRead() values that are detected when each button is pressed. This must be in order of lowest button analogRead() value to highest
    // debounceDuration - milliseconds that a button must be continually down to count as a press
    // analogResolution - nearly always 1024, but sometimes people use different analog input resolutions
    
    AnalogMultiButton(int8_t pin, int8_t total, const int16_t values[], uint16_t debounceDuration = 20, uint16_t analogResolution = 1024);

    boolean isPressed(int8_t button) { return buttonPressed == button; } // evaluates to true continually while <button> is pressed
	boolean isPressedBefore(int8_t button, int16_t duration); // called continually while <button> is pressed for less than <duration> (ms)
    boolean isPressedAfter(int8_t button, int16_t duration); // called continually while <button> is pressed for longer than <duration> (ms)
    boolean onPress(int8_t button) { return buttonOnPress == button; } // evaluates to true for one update cycle after <button> is pressed
    boolean onPressAfter(int8_t button, int16_t duration); // evaluates to true for one update cycle after <button> is pressed for longer than <duration> (ms)
	boolean onPressAndAfter(int8_t button, int16_t duration); // evaluates to true for one update cycle after <button> is pressed, and again once it has been pressed for longer than <duration> (ms)
	boolean onPressAfter(int8_t button, int16_t duration, int16_t repeatTime); // evaluates to true for one update cycle after <button> is pressed for longer than <duration>, and then repeatedly after that every <repeatTime> milliseconds
    boolean onPressAndAfter(int8_t button, int16_t duration, int16_t repeatTime); // evaluates to true for one update cycle after <button> is pressed, again when pressed for longer than <duration>, and then repeatedly after that every <repeatTime> milliseconds
	boolean onRelease(int8_t button)  { return buttonOnRelease == button; }  // evaluates to true for one update cycle after <button> is released
    boolean onReleaseBefore(int8_t button, int16_t duration);  // evaluates to true for one update cycle after <button> is released, only if it was pressed for shorter than <duration>
	boolean onReleaseAfter(int8_t button, int16_t duration);  // evaluates to true for one update cycle after <button> is released, only if it was pressed for longer than or equal to <duration>
    int32_t getPressDuration(); // gets the duration that the current button has been pressed for, in milliseconds
    int32_t getLastReleasePressDuration() { return millis() - releasedButtonPressTime; } // gets the duration that the last released button was pressed for, in milliseconds
    
    void update();
    
  private:
    int8_t pin;
    int8_t total;
    uint16_t analogResolution;
    uint16_t debounceDuration;
    int8_t valueBoundaries[AnalogMultiButton::MAX_BUTTONS];

    int8_t buttonPressed = -1;
    int8_t buttonOnPress = -1;
    int8_t buttonOnRelease = -1;

    uint32_t thisUpdateTime = 0;
    uint32_t lastUpdateTime = 0;
    uint32_t buttonPressTime = 0;
    uint32_t releasedButtonPressTime = 0;
    int8_t lastDebounceButton = -1;
    uint32_t lastDebounceButtonTime = 0;

    int8_t getButtonForAnalogValue(int16_t value);
    boolean debounceButton(int8_t button);
};

#endif
