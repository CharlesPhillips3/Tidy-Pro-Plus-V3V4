// The Shelf crash recorder (the same as TidyOS Pro's). A crash's report goes to the board's own serial port,
// which the USB console doesn't show, so the panic handler is wrapped (-Wl,--wrap=esp_panic_handler) to copy
// the reason and a short backtrace into RTC memory. That survives the restart, and the next start prints it
// on the USB console as "[shelf] last run crashed: ..." (look the addresses up against the firmware's .elf).
#include <Arduino.h>
#include "esp_attr.h"
#include "esp_system.h"
#include "esp_private/panic_internal.h"
#include "esp_debug_helpers.h"
#include "xtensa/xtensa_context.h"
#include "board.h"

#define CRASH_MAGIC 0x5E1FC0DEu
struct CrashRec { uint32_t magic; int32_t core; char reason[48]; uint32_t pc[12]; int32_t n; uint32_t upSec; };
static RTC_NOINIT_ATTR CrashRec s_rec;
static char s_text[220];

extern "C" {
void __real_esp_panic_handler(panic_info_t * info);
IRAM_ATTR void __wrap_esp_panic_handler(panic_info_t * info)
{
  s_rec.magic = CRASH_MAGIC;
  s_rec.core = info->core;
  s_rec.upSec = (uint32_t)(esp_timer_get_time() / 1000000);
  const char * r = info->reason; int i = 0;
  if (r) for (; i < 47 && r[i]; ++i) s_rec.reason[i] = r[i];
  s_rec.reason[i] = 0;
  s_rec.n = 0;
  if (info->frame) {
    const XtExcFrame * f = (const XtExcFrame *)info->frame;
    esp_backtrace_frame_t fr = { (uint32_t)f->pc, (uint32_t)f->a1, (uint32_t)f->a0, f };
    s_rec.pc[s_rec.n++] = fr.pc;
    while (s_rec.n < 12 && fr.next_pc && esp_backtrace_get_next_frame(&fr)) s_rec.pc[s_rec.n++] = fr.pc;
  }
  __real_esp_panic_handler(info);
}
}

void crashBegin()                    // first thing in setup(): keep the record, clear it for next time
{
  s_text[0] = 0;
  if (s_rec.magic == CRASH_MAGIC) {
    s_rec.reason[47] = 0;
    int n = snprintf(s_text, sizeof s_text, "%s on core %d after %um %02us at", s_rec.reason, (int)s_rec.core,
                     (unsigned)(s_rec.upSec / 60), (unsigned)(s_rec.upSec % 60));
    for (int i = 0; i < s_rec.n && i < 12 && n > 0 && n < (int)sizeof s_text - 12; ++i)
      n += snprintf(s_text + n, sizeof s_text - n, " 0x%08x", (unsigned)s_rec.pc[i]);
  }
  s_rec.magic = 0;
}
void crashReport()                   // once the USB console has had time to reconnect
{
  if (s_text[0]) Serial.printf("[shelf] last run crashed: %s (The Shelf " FW_VERSION ")\n", s_text);
}
