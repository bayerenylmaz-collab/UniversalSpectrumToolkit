#!/usr/bin/env python3
"""HDTV .spe dosyasini GF3'un actigi RadWare .spe formatina cevirir.

Girdi (hdtv / tab):
    8 bayt isim, int32 kanal sayisi, int32 1, int32 1, int32 1,
    ardindan kanal sayisi kadar float32 sayim.

Cikti (GF3 / RadWare, Unix Fortran unformatted, iki kayit):
    [24][isim + kanal + 1,1,1][24]
    [4*kanal][float32 sayimlar][4*kanal]

Kullanim:
    python3 hdtv_to_gf3.py
    python3 hdtv_to_gf3.py giris.spe
    python3 hdtv_to_gf3.py giris.spe cikis.spe
"""

import struct
import sys
from pathlib import Path


def read_hdtv(path):
    data = Path(path).read_bytes()
    if len(data) < 24:
        raise ValueError(f"{path}: dosya cok kisa")
    name = data[:8]
    nch, i1, i2, i3 = struct.unpack_from("<4i", data, 8)
    spec = data[24 : 24 + 4 * nch]
    if nch <= 0 or len(spec) != 4 * nch or len(data) != 24 + 4 * nch:
        raise ValueError(
            f"{path}: hdtv basligi degil (kanal={nch}, boyut={len(data)})"
        )
    return name, nch, i1, i2, i3, spec


def write_gf3(path, name, nch, i1, i2, i3, spec):
    header = name + struct.pack("<4i", nch, i1, i2, i3)
    rec1 = len(header)  # 24
    rec2 = len(spec)  # 4 * nch
    blob = b"".join(
        (
            struct.pack("<i", rec1),
            header,
            struct.pack("<i", rec1),
            struct.pack("<i", rec2),
            spec,
            struct.pack("<i", rec2),
        )
    )
    Path(path).write_bytes(blob)


def convert(src, dst):
    name, nch, i1, i2, i3, spec = read_hdtv(src)
    write_gf3(dst, name, nch, i1, i2, i3, spec)
    title = name.decode("ascii", "replace").rstrip(" \x00")
    print(f"{src} -> {dst}   ({title}, {nch} kanal)")


def default_inputs():
    here = Path(__file__).resolve().parent
    return sorted(here.glob("*_tab_hdtv.spe"))


def main(argv):
    if not argv:
        files = default_inputs()
        if not files:
            print("Kullanim: python3 hdtv_to_gf3.py giris.spe [cikis.spe]")
            return 1
        for src in files:
            dst = src.with_name(src.name.replace("_tab_hdtv", "_gf3"))
            convert(src, dst)
        return 0
    if len(argv) == 1:
        src = Path(argv[0])
        convert(src, src.with_name(src.stem + "_gf3.spe"))
        return 0
    if len(argv) == 2:
        convert(argv[0], argv[1])
        return 0
    print("Kullanim: python3 hdtv_to_gf3.py giris.spe [cikis.spe]")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
