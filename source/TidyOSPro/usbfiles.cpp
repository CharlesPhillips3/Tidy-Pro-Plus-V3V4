// TidyOS Pro - copying files over the USB cable (the web flasher's "Copy files over USB").
//
// The page talks to the board over the same USB serial port as the console, one line at a time.
// Its lines start with "~" (the serial keyboard hands them here instead of typing them):
//   ~roots                       the places files can go (SD card, the board's own storage) and their free space
//   ~ls <dir>                    a folder's contents
//   ~mkdir <dir>   ~rm <path>    make a folder, delete a file (or an empty folder)
//   ~put <path> <size>           start writing a file (to <path>.part)
//   ~w <base64>                  the next piece of it (up to ~3 KB); answered "ok w <bytes so far>"
//   ~end                         finish: <path>.part becomes <path>
//   ~get <path>                  read a file back: "d <base64>" lines, then "end <size>"
// Every answer is a line starting with \x02 "TF ", so the page can tell it from the board's log lines.
#include "core.h"
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include "mbedtls/base64.h"

static QueueHandle_t s_q = nullptr;

static void say(const char * fmt, ...)
{
  static char b[2400];                         // (only the usbfs task writes answers)
  b[0] = 2; b[1] = 'T'; b[2] = 'F'; b[3] = ' ';
  va_list ap; va_start(ap, fmt);
  int n = vsnprintf(b + 4, sizeof(b) - 6, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  n = 4 + (n > (int)sizeof(b) - 6 ? (int)sizeof(b) - 6 : n);
  b[n++] = '\n';
  Serial.write((const uint8_t *)b, n);         // one write per line: log lines from other tasks can't land inside it
  Serial.flush();
}

// only under the SD card or the board's own storage, and never ".." (a folder above them)
static bool okPath(const std::string & p)
{
  if (p.find("..") != std::string::npos || p.find('\t') != std::string::npos) return false;
  auto under = [&](const char * root) { size_t n = strlen(root); return p.compare(0, n, root) == 0 && (p.size() == n || p[n] == '/'); };
  return (gSDMounted && under(SD_MOUNT)) || (gFlashMounted && under(FLASH_MOUNT));
}

static FILE * s_put = nullptr;
static std::string s_putPath;
static size_t s_putSize = 0, s_putDone = 0;

static void endPut(bool keep)
{
  if (!s_put) return;
  fclose(s_put); s_put = nullptr;
  std::string part = s_putPath + ".part";
  if (keep) { remove(s_putPath.c_str()); rename(part.c_str(), s_putPath.c_str()); }
  else remove(part.c_str());
}

static void handle(const std::string & line)
{
  size_t sp = line.find(' ');
  std::string cmd = line.substr(0, sp), arg = sp == std::string::npos ? "" : line.substr(sp + 1);
  if (cmd == "w") {
    if (!s_put) { say("err nothing is being written"); return; }
    static unsigned char buf[3200];
    size_t n = 0;
    if (mbedtls_base64_decode(buf, sizeof(buf), &n, (const unsigned char *)arg.data(), arg.size()) != 0) { endPut(false); say("err a piece arrived damaged - try again"); return; }
    if (fwrite(buf, 1, n, s_put) != n) { endPut(false); say("err couldn't write - is the storage full?"); return; }
    s_putDone += n;
    say("ok w %u", (unsigned)s_putDone);
  } else if (cmd == "put") {
    endPut(false);
    size_t s2 = arg.rfind(' ');
    if (s2 == std::string::npos) { say("err put needs a name and a size"); return; }
    s_putPath = arg.substr(0, s2); s_putSize = strtoul(arg.c_str() + s2 + 1, nullptr, 10); s_putDone = 0;
    if (!okPath(s_putPath)) { say("err not a place files can go"); return; }
    int64_t t = 0, u = 0;
    bool sd = s_putPath.compare(0, strlen(SD_MOUNT), SD_MOUNT) == 0;
    if ((sd ? FileBrowser::getFSInfo(fabgl::DriveType::SDCard, 0, &t, &u) : internalFSInfo(t, u)) && (int64_t)s_putSize > t - u) {
      say("err not enough room: %u KB free, the file is %u KB", (unsigned)((t - u) / 1024), (unsigned)(s_putSize / 1024)); return;
    }
    s_put = fopen((s_putPath + ".part").c_str(), "wb");
    if (!s_put) { say("err couldn't create the file (does the folder exist?)"); return; }
    say("ok put");
  } else if (cmd == "end") {
    if (!s_put) { say("err nothing is being written"); return; }
    bool whole = s_putDone == s_putSize;
    endPut(whole);
    if (whole) say("ok end %u", (unsigned)s_putDone); else say("err only %u of %u bytes arrived", (unsigned)s_putDone, (unsigned)s_putSize);
  } else if (cmd == "roots") {
    int64_t t = 0, u = 0;
    if (gSDMounted && FileBrowser::getFSInfo(fabgl::DriveType::SDCard, 0, &t, &u)) say("root SD card\t" SD_MOUNT "\t%lld\t%lld", (long long)(t - u), (long long)t);
    if (gFlashMounted && internalFSInfo(t, u)) say("root Board storage\t" FLASH_MOUNT "\t%lld\t%lld", (long long)(t - u), (long long)t);
    say("end");
  } else if (cmd == "ls") {
    if (!okPath(arg)) { say("err not a place files can go"); return; }
    DIR * d = opendir(arg.c_str());
    if (!d) { say("err couldn't open that folder"); return; }
    while (dirent * e = readdir(d)) {
      if (e->d_name[0] == '.') continue;
      std::string p = arg + "/" + e->d_name;
      struct stat st;
      bool dir = e->d_type == DT_DIR;
      long sz = 0;
      if (!dir && stat(p.c_str(), &st) == 0) sz = st.st_size;
      say("e %c\t%ld\t%s", dir ? 'd' : 'f', sz, e->d_name);
    }
    closedir(d);
    say("end");
  } else if (cmd == "mkdir") {
    if (!okPath(arg)) { say("err not a place files can go"); return; }
    if (mkdir(arg.c_str(), 0777) == 0) say("ok mkdir"); else say("err couldn't make that folder");
  } else if (cmd == "rm") {
    if (!okPath(arg) || arg == SD_MOUNT || arg == FLASH_MOUNT) { say("err can't delete that"); return; }
    if (remove(arg.c_str()) == 0 || rmdir(arg.c_str()) == 0) say("ok rm"); else say("err couldn't delete it (a folder has to be empty)");
  } else if (cmd == "get") {
    if (!okPath(arg)) { say("err not a place files can go"); return; }
    FILE * f = fopen(arg.c_str(), "rb");
    if (!f) { say("err couldn't open that file"); return; }
    static unsigned char raw[1500]; static unsigned char enc[2100];
    size_t n, total = 0, olen;
    while ((n = fread(raw, 1, sizeof(raw), f)) > 0) {
      mbedtls_base64_encode(enc, sizeof(enc), &olen, raw, n);
      enc[olen] = 0;
      say("d %s", (const char *)enc);
      total += n;
    }
    fclose(f);
    say("end %u", (unsigned)total);
  } else {
    say("err unknown command");
  }
}

static void usbFilesTask(void *)
{
  for (;;) {
    char * l = nullptr;
    if (xQueueReceive(s_q, &l, portMAX_DELAY) == pdTRUE && l) { handle(l); free(l); }
  }
}

void usbFilesLine(const std::string & line)       // from the serial keyboard (input.cpp), a line after its "~"
{
  if (!s_q) {
    s_q = xQueueCreate(4, sizeof(char *));
    xTaskCreatePinnedToCore(usbFilesTask, "usbfs", 8192, nullptr, 2, nullptr, 1);
  }
  char * l = strdup(line.c_str());
  if (l && xQueueSend(s_q, &l, pdMS_TO_TICKS(2000)) != pdTRUE) free(l);
}
