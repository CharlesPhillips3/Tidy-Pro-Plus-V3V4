// TidyOS in-app update from GitHub (see update.cpp)
#pragma once
#include <string>
bool updateLoaderPresent();                 // the board has the update loader (installed by the web flasher)
void updateRequest();          // restart into update mode (in colour with PSRAM; black & white without, colours come back after)
bool updateRestoreColors();    // at a normal start: put the colour setting back after an update                       // restart into update mode
bool updateModeRequested();
void updateRequestPhone();     // like updateRequest(), but the update comes through the phone's TidyOS page
int  updatePhoneCheck();       // can the phone showing the desktop reach GitHub? 200 = yes (-20 = no phone page open)                 // true once after updateRequest() (clears the flag)
void updateRun(void (*status)(const char * msg, int percent));   // never returns: restarts
