#!/usr/bin/env python3
"""HDTV .spe dosyasini GF3 .spe dosyasina cevirir.

Dosyayi acmaz. Cift tiklamak veya python'a .spe vermek Windows'ta
"hangi programla acilsin" penceresini acar. Bu betik sadece yeni bir
dosya yazar. Cikti, girdinin yanina kaydedilir.

    python hdtv_to_gf3.py
    python hdtv_to_gf3.py "C:\\Users\\PC\\Desktop\\27Si_17_tab_hdtv.spe"
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


def output_path(src):
    src = Path(src).resolve()
    name = src.name
    if name.endswith("_tab_hdtv.spe"):
        name = name[: -len("_tab_hdtv.spe")] + "_gf3.spe"
    elif name.endswith(".spe"):
        name = name[:-4] + "_gf3.spe"
    else:
        name = name + "_gf3.spe"
    return src.parent / name


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
    dst, size = save_bytes(output_path(src), blob)
    vals = struct.unpack("<%df" % nch, spec)
    print(f"Kaydedildi, acilmadi: {dst}")
    print(f"  {size} bayt, {nch} kanal")
    return {
        "src": str(src.resolve()),
        "dst": str(dst),
        "name": title_of(name),
        "nch": nch,
        "nbytes": size,
        "first": vals[0],
        "lo": min(vals),
        "hi": max(vals),
    }


def save_report(rows):
    by_dir = {}
    for row in rows:
        by_dir.setdefault(Path(row["dst"]).parent, []).append(row)
    for folder, group in by_dir.items():
        lines = ["GF3 SPE cikti kaydi", "Dosya acilmadi, sadece kaydedildi.", ""]
        for row in group:
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
        path, _ = save_bytes(folder / "rapor.txt", "\n".join(lines).encode("utf-8"))
        print(f"Kaydedildi, acilmadi: {path}")


def search_roots():
    home = Path.home()
    roots = [
        Path.cwd(),
        HERE,
        home / "Desktop",
        home / "Masaüstü",
        home / "Downloads",
        home / "OneDrive" / "Desktop",
        home / "OneDrive" / "Masaüstü",
    ]
    unique = []
    seen = set()
    for root in roots:
        try:
            key = root.resolve()
        except OSError:
            continue
        if key in seen or not root.is_dir():
            continue
        seen.add(key)
        unique.append(root)
    return unique


def is_input(path):
    name = path.name.lower()
    return name.endswith(".spe") and not name.endswith("_gf3.spe")


def find_inputs(argv):
    if argv:
        return [Path(item) for item in argv]
    found = []
    seen = set()
    for root in search_roots():
        candidates = [root / name for name in KNOWN_INPUTS]
        candidates.extend(sorted(root.glob("*_tab_hdtv.spe")))
        for path in candidates:
            if not path.is_file() or not is_input(path):
                continue
            key = path.resolve()
            if key in seen:
                continue
            seen.add(key)
            found.append(path)
    return found


def main(argv):
    files = find_inputs(argv)
    if not files:
        print("HDTV .spe bulunamadi.")
        print('Ornek: python hdtv_to_gf3.py "C:\\Users\\PC\\Desktop\\27Si_17_tab_hdtv.spe"')
        print("python.exe dosya.spe yazmayin.")
        print("O komut spektrumu program sanir ve null bytes hatasi verir.")
        return 1
    rows = [convert(path) for path in files]
    save_report(rows)
    print("Bitti. Olusan .spe dosyasina cift tiklamayin.")
    print("Windows acma penceresi cikarsa kapatin; dosya zaten kayitlidir.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
