#include <Arduino.h>
#include <unity.h>
#include "MemoryFree.h"



void test_led_builtin_pin_number(void) {
    TEST_ASSERT_EQUAL(13, LED_BUILTIN);
}

void test_led_state_high(void) {
    digitalWrite(LED_BUILTIN, HIGH);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_BUILTIN));
}

void test_led_state_low(void) {
    digitalWrite(LED_BUILTIN, LOW);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_BUILTIN));
}


void ioParams(uint32_t aa, uint32_t bb, uint32_t *a, uint32_t *b)
{   
    *a = aa + 1;
    *b = bb + 1;
    //*uiData &= ~(uint32_t)0xFFFF;
    //*uiFlashData &= ~(uint32_t)0xFFFF;
    //*uiData |= (uint32_t)displayData;
    //*uiFlashData |= (uint32_t)flashData;
}


void test_passing_vars(void) {
    uint32_t a = 10, b = 20;

    ioParams(a, b, &a, &b);

    TEST_ASSERT_EQUAL(11, a);
    TEST_ASSERT_EQUAL(21, b);

}


void setup() {    
    delay(2000);    

    UNITY_BEGIN();     
    RUN_TEST(test_passing_vars);
    UNITY_END();
}


void loop() {
    /*
    if (i < max_blinks)
    {
        Serial.println("this is a test");
        RUN_TEST(test_led_state_high);
        delay(500);
        RUN_TEST(test_led_state_low);
        delay(500);
        i++;
    }
    else if (i == max_blinks) {
      UNITY_END(); // stop unit testing
    }
    */
}