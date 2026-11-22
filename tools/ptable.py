#!/usr/bin/env python3
"""Make an ESP32 partition table binary from a partitions.csv (same bytes as ESP-IDF's gen_esp32part.py).
usage: ptable.py partitions.csv > table.bin"""
import struct, hashlib, sys
T={'app':0,'data':1}; S={'factory':0,'ota_0':0x10,'ota_1':0x11,'nvs':2,'ota':0,'spiffs':0x82,'fat':0x81,'coredump':3}
def build(rows):
    out=b''
    for name,typ,sub,off,size in rows:
        out+=struct.pack('<2sBBLL16sL',b'\xaa\x50',T[typ],S[sub],off,size,name.encode().ljust(16,b'\0'),0)
    out+=b'\xeb\xeb'+b'\xff'*14+hashlib.md5(out).digest()
    return out.ljust(0xC00,b'\xff')
def parse(csv):
    r=[]
    for l in open(csv):
        l=l.split('#')[0].strip()
        if not l: continue
        a=[x.strip() for x in l.split(',')]
        r.append((a[0],a[1],a[2],int(a[3],0),int(a[4],0)))
    return r
if __name__=='__main__':
    sys.stdout.buffer.write(build(parse(sys.argv[1])))
