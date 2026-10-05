#!/usr/bin/env python3
"""HDTV .spe dosyasini GF3 .spe dosyasina cevirir ve diske kaydeder.

Calistirinca ciktiyi kendisi yazar. Cikti adi sorulmaz.
Kayit yeri, calistirildigi klasordeki gf3_cikti/ dizinidir.

    python3 hdtv_to_gf3.py
    python3 hdtv_to_gf3.py giris.spe
"""

import os
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
KNOWN_INPUTS = (
    "23Mg_17_tab_hdtv.spe",
    "27Si_17_tab_hdtv.spe",
    "31S_17_tab_hdtv.spe",
)


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


def gf3_bytes(name, nch, i1, i2, i3, spec):
    header = name + struct.pack("<4i", nch, i1, i2, i3)
    rec1 = len(header)
    rec2 = len(spec)
    return b"".join(
        (
            struct.pack("<i", rec1),
            header,
            struct.pack("<i", rec1),
            struct.pack("<i", rec2),
            spec,
            struct.pack("<i", rec2),
        )
    )


def output_dir():
    folder = Path.cwd() / "gf3_cikti"
    folder.mkdir(parents=True, exist_ok=True)
    return folder


def output_name(src):
    name = Path(src).name
    if name.endswith("_tab_hdtv.spe"):
        return name[: -len("_tab_hdtv.spe")] + "_gf3.spe"
    if name.endswith(".spe"):
        return name[:-4] + "_gf3.spe"
    return name + "_gf3.spe"


def save_bytes(path, blob):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(blob)
        handle.flush()
        os.fsync(handle.fileno())
    size = path.stat().st_size
    if size != len(blob):
        raise RuntimeError(f"kayit eksik: {path} ({size} bayt)")
    return path.resolve(), size


def title_of(name):
    return name.decode("ascii", "replace").rstrip(" \x00")


def convert(src):
    src = Path(src)
    name, nch, i1, i2, i3, spec = read_hdtv(src)
    blob = gf3_bytes(name, nch, i1, i2, i3, spec)
    dst, size = save_bytes(output_dir() / output_name(src), blob)
    vals = struct.unpack("<%df" % nch, spec)
    print(f"kaydedildi: {dst} ({size} bayt, {nch} kanal)")
    return {
        "src": src.name,
        "dst": str(dst),
        "name": title_of(name),
        "nch": nch,
        "i1": i1,
        "i2": i2,
        "i3": i3,
        "nbytes": size,
        "first": vals[0],
        "lo": min(vals),
        "hi": max(vals),
    }


def save_report(rows):
    lines = ["GF3 SPE cikti kaydi", ""]
    for row in rows:
        lines.append(f"dosya: {Path(row['dst']).name}")
        lines.append(f"  yol: {row['dst']}")
        lines.append(f"  kaynak: {row['src']}")
        lines.append(f"  isim: {row['name']}")
        lines.append(f"  kanal: {row['nch']}")
        lines.append(f"  dosya boyu: {row['nbytes']} bayt")
        lines.append(f"  ilk kanal: {row['first']:.1f}")
        lines.append(f"  en kucuk: {row['lo']:.1f}")
        lines.append(f"  en buyuk: {row['hi']:.1f}")
        lines.append("")
    text = "\n".join(lines)
    path, _ = save_bytes(output_dir() / "rapor.txt", text.encode("utf-8"))
    print(f"kaydedildi: {path}")


def find_inputs(argv):
    if argv:
        return [Path(item) for item in argv]
    found = []
    seen = set()
    for root in (Path.cwd(), HERE):
        for name in KNOWN_INPUTS:
            path = root / name
            if path.is_file() and path.resolve() not in seen:
                seen.add(path.resolve())
                found.append(path)
        for path in sorted(root.glob("*_tab_hdtv.spe")):
            if path.resolve() not in seen:
                seen.add(path.resolve())
                found.append(path)
    return found


def main(argv):
    files = find_inputs(argv)
    if not files:
        print("HDTV .spe bulunamadi. Calistirildigi klasore koyun.")
        return 1
    rows = [convert(path) for path in files]
    save_report(rows)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
