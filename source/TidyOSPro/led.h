// TidyOS status light (Heltec V4: the white LED on GPIO 35)
#pragma once
#include <stdint.h>
void ledBegin();      // start blinking (call first thing in setup)
void ledReady();      // TidyOS is up: switch to breathing
void ledSetOn(bool on);   // the Switches window (off by default)
extern volatile uint32_t gSdActivityMs;
