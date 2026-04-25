#pragma once

#include <cstdint>

#define BRAIN_USE_ALL 1
#include "brain/brain.h"

enum TestId : uint8_t {
    kTestLeds = 0,
    kTestPot1,
    kTestPot2,
    kTestPot3,
    kTestButtonLed,
    kTestButtonB,
    kTestMidi,
    kTestCvIn1,
    kTestCvIn2,
    kTestPulseIn,
    kTestCvOut1,
    kTestCvOut2,
    kTestPulseOut,
    kTestCvOutCalibrate,
    kTestCount,
};

void on_test_enter(Brain& brain, TestId test);
void run_test(Brain& brain, TestId test, uint32_t now_ms);

void midi_note_on(uint8_t note, uint8_t velocity, uint8_t channel);
void midi_note_off(uint8_t note, uint8_t velocity, uint8_t channel);

void button_b_press();
void button_b_release();
