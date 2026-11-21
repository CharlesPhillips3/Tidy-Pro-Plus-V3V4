#!/usr/bin/env python3
"""Tidy Pro Plus: split the full images into one file per piece, for the pick-what-you-install flasher.

  firmware/parts/<b>/menu.bin    boot menu (TidyBoot)          -> 0x10000
  firmware/parts/<b>/shelf.bin   The Shelf (dual-boot build)   -> 0x380000
  firmware/parts/<b>/www.bin     The Shelf's web app           -> 0x580000
  TidyOS Pro itself is firmware/tidyos-pro-<b>[-full].bin       -> 0x80000
  parts.json                     sizes and offsets the flasher reads, and the app list

App images are cut after their last non-blank 4 KB sector (blank flash is 0xFF, so nothing is lost).
usage: parts.py   (run after package.py)
"""
import os, json
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
fw = os.path.join(root, 'firmware')
BASE = 0xE000
SLOTS = {'menu': (0x10000, 0x80000), 'tidyos': (0x80000, 0x380000), 'shelf': (0x380000, 0x580000), 'www': (0x580000, 0x780000)}
def trim(b):
    n = len(b.rstrip(b'\xff')); n = (n + 4095) // 4096 * 4096
    return b[:n]
out = {'flash': 16 * 1048576, 'boards': {}}
for b in ('v4', 'v4-r8'):
    dual = os.path.join(fw, f'pro-{b}-dual-main.bin')
    if not os.path.exists(dual): continue
    d = open(dual, 'rb').read()
    pd = os.path.join(fw, 'parts', b); os.makedirs(pd, exist_ok=True)
    info = {'boot': {'path': f'firmware/pro-{b}-boot.bin', 'offset': 0, 'size': os.path.getsize(os.path.join(fw, f'pro-{b}-boot.bin'))}}
    for name, (a, e) in SLOTS.items():
        img = d[a - BASE:e - BASE]
        img = img if name == 'www' else trim(img)
        if name == 'tidyos':   # published on their own already (field and full builds): reuse them
            for k, p in (('tidyos', f'firmware/tidyos-pro-{b}.bin'), ('tidyosFull', f'firmware/tidyos-pro-{b}-full.bin')):
                if os.path.exists(os.path.join(root, p)):
                    app = open(os.path.join(root, p), 'rb').read()
                    m = app.find(b'TIDYPLUS-APPS-1\0')
                    assert m > 0, p + ': no app list in it'
                    info[k] = {'path': p, 'offset': a, 'size': len(app), 'slot': e - a, 'appList': m}
            continue
        open(os.path.join(pd, name + '.bin'), 'wb').write(img)
        info[name] = {'path': f'firmware/parts/{b}/{name}.bin', 'offset': a, 'size': len(img), 'slot': e - a}
    out['boards'][b] = info
    print(b, {k: v['size'] for k, v in info.items()})
# The desktop apps. bit = the firmware's bit number (core.cpp ALL_ICONS); full = only in the full build;
# kb = about how much flash the app adds (measured: full build with and without it).
out['apps'] = [
  # field tools
  ('RF Sweep', 0, 'field', 0, 0), ('Meter Traffic', 1, 'field', 0, 0), ('Site Survey', 2, 'field', 0, 0),
  ('RF Exposure', 3, 'field', 0, 0), ('GPS', 4, 'field', 0, 0), ('Clone', 5, 'field', 0, 0), ('Wi-Fi Scan', 6, 'field', 0, 0),
  # everyday TidyOS apps that are in both builds
  ('Tidy', 9, 'core', 0, 0), ('Neat', 10, 'core', 0, 0), ('Files', 11, 'core', 0, 0), ('Calculator', 12, 'core', 0, 0),
  ('Browser', 13, 'core', 0, 0), ('Messages', 14, 'core', 0, 0), ('Clock', 15, 'core', 0, 0), ('Nic Sweep', 7, 'core', 0, 0),
  ('Ping', 8, 'core', 0, 0), ('Screenshot', 16, 'core', 0, 0), ('Git-Store', 17, 'core', 0, 0), ('Task Manager', 19, 'core', 0, 0),
  ('Switches', 20, 'core', 0, 0), ('The Shelf', 21, 'core', 0, 0), ('Settings', 18, 'core', 0, 0),
  # the rest of TidyOS-ESP32: only in the full build
  ('Ready', 22, 'extra', 1, 4), ('Steady', 23, 'extra', 1, 8), ('Rugged', 24, 'extra', 1, 171), ('Latch', 29, 'extra', 1, 30),
  ('Games', 25, 'extra', 1, 51), ('Doodle', 26, 'extra', 1, 11), ('Pictures', 27, 'extra', 1, 1), ('Music', 28, 'extra', 1, 1),
  ('HTML Games', 30, 'extra', 1, 1), ('HTML Programs', 31, 'extra', 1, 1),
]
out['apps'] = [dict(zip(('name', 'bit', 'group', 'full', 'kb'), a)) for a in out['apps']]
# what the flasher installs: versions and build date (shown on the page)
import re, datetime
cfg = open(os.path.join(root, 'source/TidyOSPro/config.h')).read()
out['version'] = re.search(r'#define TIDYOS_VERSION "([^"]+)"', cfg).group(1)
try: out['shelfVersion'] = json.load(open(os.path.join(root, 'shelf/ota/shelf-v4-dual.json')))['version']
except Exception: out['shelfVersion'] = ''
out['built'] = datetime.date.today().isoformat()
json.dump(out, open(os.path.join(root, 'parts.json'), 'w'), indent=2)
