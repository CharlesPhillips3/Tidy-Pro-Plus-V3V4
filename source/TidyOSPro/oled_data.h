// Tidy Pro Plus: what the V4's OLED pages show, copied from the desktop's live data (desk_panel.h).
#pragma once
#include <stdint.h>

struct DeskSeries { int16_t v[40]; uint8_t ok[40]; int n; };

struct DeskSnap {
  bool wxOk; int temp, hi, lo; char cond[24]; char city[32];
  int8_t temps[40]; int tempsN;
  DeskSeries wind, gust, hum, cape, thun, duct, kp;     // every 3 h over 4 days (kp is Kp x 10)
  bool haveNow; int windNow, gustNow, windDir, rh, dew; float inHg;
  int sfi; char xray[8];
  bool hfOk; const char * hf[6];                          // 160 80 40 20 15 10 m: "Good" "Fair" "Poor" "Local"
  int alertN; char alert[3][64];                          // -1 looking, -2 unreachable
  int unread;                                             // messages not read yet
  int newsN; char news[6][120];                           // headlines (NPR, or BBC)
};
void deskSnapshot(DeskSnap & d);                          // core.cpp (desk_panel.h)
