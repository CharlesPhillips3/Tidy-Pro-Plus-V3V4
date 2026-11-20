#!/usr/bin/env python3
"""Tidy Pro Plus: split the full images into one file per piece, for the pick-what-you-install flasher.

  firmware/parts/<b>/menu.bin    boot menu (TidyBoot)          -> 0x10000
  firmware/parts/<b>/shelf.bin   The Shelf (dual-boot build)   -> 0x380000
  firmware/parts/<b>/www.bin     The Shelf's web app           -> 0x580000
  TidyOS Pro itself is firmware/tidyos-pro-<b>.bin             -> 0x80000
  parts.json                     sizes and offsets the flasher reads

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
        if name == 'tidyos':   # already published on its own for the Wi-Fi updater: reuse it
            p = f'firmware/tidyos-pro-{b}.bin'; app = open(os.path.join(root, p), 'rb').read()
            assert img.startswith(app)
            info[name] = {'path': p, 'offset': a, 'size': len(app), 'slot': e - a}
            continue
        open(os.path.join(pd, name + '.bin'), 'wb').write(img)
        info[name] = {'path': f'firmware/parts/{b}/{name}.bin', 'offset': a, 'size': len(img), 'slot': e - a}
    out['boards'][b] = info
    print(b, {k: v['size'] for k, v in info.items()})
json.dump(out, open(os.path.join(root, 'parts.json'), 'w'), indent=2)
