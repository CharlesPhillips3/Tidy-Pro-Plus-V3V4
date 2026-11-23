// TidyOS HTML5 edition: the desktop is shown and controlled in a web browser
#pragma once
#include <string>
#include <functional>
#include <stdint.h>
#include <stddef.h>

#define SCREEN_AP_NAME (boardId().c_str())   // each board its own hotspot name, e.g. "TidyOS-A93160"
#define SCREEN_AP_PASS "tidyos123"

#ifdef TIDYOS_HTML5
void        screenBegin();       // start the web screen server (+ setup hotspot when needed)
bool        screenViewer();      // a browser is connected
bool        screenApOn();        // the TidyOS-Setup hotspot is running
bool        screenStaPaused();   // not looking for home Wi-Fi while devices use the hotspot
// a file fetched by the browser showing the screen, over its own internet: HTTP status, or < 0
int         screenRelayGet(const char * url, const std::function<bool(const uint8_t *, size_t, size_t)> & sink);
void        screenHotspot(bool on); // keep the hotspot running (Settings)
void        screenTrim();           // free what the web screen isn't using
std::string screenUrl();         // where to open the desktop, e.g. http://192.168.1.40
// sound and full-colour pictures play in the browser:
//   "a <path>" play audio, "p" pause, "r" resume, "s" stop, "v 0.5" volume, "k 63" seek (s),
//   "i <path>" show a picture full size, "x" close it.  <path> is on the file server's port.
//   "m <path>" play a MIDI file (synthesised in the browser; p/r/s/v work on it too)
//   "o <path>\t<name>" open a web page from the card in a new browser tab (HTML games / programs)
//   "g <op>\t<relay>\t<board>..." Git-Store: the page talks to the relay and moves files (reply: message 13)
//   "u go" / "u nosd" answer to files dropped on the page (message 15); each saved file is reported with message 16
//   "w" look up the weather and headlines now (messages 12 and 14)
//   "b 1" ring the alarm (beeps for up to 2 minutes), "b 0" stop it, "b 2" one beep.
void        screenMedia(const std::string & cmd);
void        screenMediaState(int & state, int & pos, int & dur);   // state 0 paused 1 playing 2 ended -1 none
int         screenMediaEpoch();           // changes with every new browser page (its music is gone with the old one)
#else
inline void        screenBegin()  {}
inline bool        screenViewer() { return false; }
inline bool        screenApOn()   { return false; }
inline bool        screenStaPaused() { return false; }
inline int         screenRelayGet(const char *, const std::function<bool(const uint8_t *, size_t, size_t)> &) { return -10; }
inline void        screenHotspot(bool) {}
inline void        screenTrim() {}
inline std::string screenUrl()    { return ""; }
inline void        screenMedia(const std::string &) {}
inline void        screenMediaState(int & s, int & p, int & d) { s = -1; p = d = 0; }
inline int         screenMediaEpoch() { return 0; }
#endif
