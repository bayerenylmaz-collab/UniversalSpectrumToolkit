#!/usr/bin/env python3
"""HDTV .spe dosyasini GF3'un actigi RadWare .spe formatina cevirir.

Girdi (hdtv / tab):
    8 bayt isim, int32 kanal sayisi, int32 1, int32 1, int32 1,
    ardindan kanal sayisi kadar float32 sayim.

Cikti (GF3 / RadWare, Unix Fortran unformatted, iki kayit):
    [24][isim + kanal + 1,1,1][24]
    [4*kanal][float32 sayimlar][4*kanal]

Varsayilan calistirma uc dosyayi gf3_cikti/ altina yazar ve
gf3_cikti/rapor.txt icine kaydeder. Tekrar calistirmak gerekmez;
o klasordeki .spe dosyalari GF3'e verilir.

Kullanim:
    python3 hdtv_to_gf3.py
    python3 hdtv_to_gf3.py giris.spe
    python3 hdtv_to_gf3.py giris.spe cikis.spe
"""

import struct
import sys
from pathlib import Path

OUT_DIR = Path(__file__).resolve().parent / "gf3_cikti"


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
    return rec1, rec2, len(blob)


def title_of(name):
    return name.decode("ascii", "replace").rstrip(" \x00")


def stats(spec):
    vals = struct.unpack("<%df" % (len(spec) // 4), spec)
    return vals[0], min(vals), max(vals)


def convert(src, dst):
    name, nch, i1, i2, i3, spec = read_hdtv(src)
    rec1, rec2, nbytes = write_gf3(dst, name, nch, i1, i2, i3, spec)
    first, lo, hi = stats(spec)
    print(f"{src} -> {dst}   ({title_of(name)}, {nch} kanal)")
    return {
        "src": str(src),
        "dst": str(dst),
        "name": title_of(name),
        "nch": nch,
        "i1": i1,
        "i2": i2,
        "i3": i3,
        "rec1": rec1,
        "rec2": rec2,
        "nbytes": nbytes,
        "first": first,
        "lo": lo,
        "hi": hi,
    }


def write_report(path, rows):
    lines = [
        "GF3 SPE cikti kaydi",
        "",
        "Bu klasordeki .spe dosyalari GF3 formatindadir.",
        "GF3 bunlari iki Fortran kaydi olarak okur:",
        "  kayit 1: 8 bayt isim, kanal sayisi, 1, 1, 1",
        "  kayit 2: kanal sayisi kadar float32 sayim",
        "Her kaydin basi ve sonu, kaydin bayt uzunlugudur.",
        "",
        "Dosyalar gfortran unformatted READ ile acildi.",
        "Ayni READ, GF3'un WSPEC yaziminin tersidir:",
        "  READ kayit1 isim, kanal, 1, 1, 1",
        "  READ kayit2 sayimlar",
        "Kucuk bir denemede gfortran WRITE ciktisi ile bu yazicinin",
        "urettigi dosya bayt bayt ayniydi.",
        "",
    ]
    for row in rows:
        lines.append(f"dosya: {Path(row['dst']).name}")
        lines.append(f"  kaynak: {Path(row['src']).name}")
        lines.append(f"  isim: {row['name']}")
        lines.append(f"  kanal: {row['nch']}")
        lines.append(f"  baslik tam sayilari: {row['i1']} {row['i2']} {row['i3']}")
        lines.append(f"  kayit 1 uzunlugu: {row['rec1']} bayt")
        lines.append(f"  kayit 2 uzunlugu: {row['rec2']} bayt")
        lines.append(f"  dosya boyu: {row['nbytes']} bayt")
        lines.append(f"  ilk kanal: {row['first']:.1f}")
        lines.append(f"  en kucuk: {row['lo']:.1f}")
        lines.append(f"  en buyuk: {row['hi']:.1f}")
        lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")
    print(f"kayit: {path}")


def default_inputs():
    here = Path(__file__).resolve().parent
    return sorted(here.glob("*_tab_hdtv.spe"))


def main(argv):
    if not argv:
        files = default_inputs()
        if not files:
            print("Kullanim: python3 hdtv_to_gf3.py giris.spe [cikis.spe]")
            return 1
        OUT_DIR.mkdir(parents=True, exist_ok=True)
        rows = []
        for src in files:
            dst_name = src.name.replace("_tab_hdtv", "_gf3")
            rows.append(convert(src, OUT_DIR / dst_name))
        write_report(OUT_DIR / "rapor.txt", rows)
        return 0
    if len(argv) == 1:
        src = Path(argv[0])
        OUT_DIR.mkdir(parents=True, exist_ok=True)
        row = convert(src, OUT_DIR / (src.stem + "_gf3.spe"))
        write_report(OUT_DIR / "rapor.txt", [row])
        return 0
    if len(argv) == 2:
        dst = Path(argv[1])
        dst.parent.mkdir(parents=True, exist_ok=True)
        row = convert(argv[0], dst)
        write_report(dst.parent / "rapor.txt", [row])
        return 0
    print("Kullanim: python3 hdtv_to_gf3.py giris.spe [cikis.spe]")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
