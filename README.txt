UNIVERSAL SPECTRUM TOOLKIT
==========================

Presentation (Turkish, Beamer PDF)
----------------------------------
  docs/presentation/ust_sunum.pdf
  Source: docs/presentation/ust_sunum.tex
  Figures: docs/presentation/figures/

================================================================
ENGLISH
================================================================

Lab helper for converting MCA spectrum files that ROOT cannot
open natively into analysis-ready ROOT files.

What it does
------------
1. Detects known spectrum layouts with registered readers
2. Writes a ROOT file with:
   - TH1D hSpectrumChannel
   - TH1D hSpectrumEnergy   (when energy calibration exists)
   - TTree spectrum         (channel, counts, energy)
   - TTree conversion_metadata
   - raw/raw_bytes          (exact original bytes, one vector entry)
3. Unknown formats are scanned and reported; they are NOT converted
   unless you explicitly pass forceGeneric=true

Current known readers
---------------------
- Tagged ASCII SPE (Ortec / Maestro style: $DATA:, $MEAS_TIM:, ...)
- GF3-like binary SPE (40-byte header + float32 little-endian)
- Ortec binary CHN (32-byte header + uint32 counts, optional energy trailer)

Easiest start (menu, no ROOT expertise needed)
----------------------------------------------
  cd /path/to/UniversalSpectrumToolkit
  ./ust_menu.sh

  Also see: BASLA.txt

Classic ROOT usage
------------------
  cd /path/to/UniversalSpectrumToolkit
  root -l
  .L universal_spectrum_to_root.C+
  universal_spectrum_help();
  universal_spectrum_to_root("samples/ortec_ascii.spe");
  universal_spectrum_to_root("samples/gf3_like.spe", "", false);
  universal_spectrum_to_root("samples/ortec_demo.chn");

Batch convert every *.spe and *.chn in a directory:
  universal_spectrum_batch("samples");

Force the best generic binary layout only when you accept the risk:
  universal_spectrum_to_root("unknown.bin", "", false, true);

Standalone reader tests (no ROOT required)
------------------------------------------
  cd tests
  g++ -std=c++17 -O2 -o test_readers test_readers.cpp
  ./test_readers

Notes
-----
- Prefer tagged ASCII Ortec SPE when available; it carries live/real
  time and energy calibration metadata.
- Binary GF3-like matching is structural and intentionally stricter.
- Ortec CHN often includes live/real time; energy calibration is used
  when the CHN trailer provides a non-zero slope.
- Raw bytes are preserved for provenance / re-analysis.


================================================================
TURKCE
================================================================

Sunum (Turkce Beamer PDF)
-------------------------
  docs/presentation/ust_sunum.pdf
  Kaynak: docs/presentation/ust_sunum.tex
  Gorseller: docs/presentation/figures/

ROOT'un dogrudan acamadigi MCA spektrum dosyalarini (ozellikle .spe
ve .chn) analiz icin hazir ROOT dosyasina ceviren bir laboratuvar yardimci aracidir.

Ne yapar?
---------
1. Kayitli okuyucularla bilinen spektrum bicimlerini tanir
2. Su nesneleri iceren bir ROOT dosyasi yazar:
   - TH1D hSpectrumChannel
   - TH1D hSpectrumEnergy   (enerji kalibrasyonu varsa)
   - TTree spectrum         (channel, counts, energy)
   - TTree conversion_metadata
   - raw/raw_bytes          (orijinal baytlarin tam kopyasi)
3. Bilinmeyen formatlari tarar ve raporlar; forceGeneric=true
   vermedikce sessizce donusturmez

Desteklenen okuyucular
----------------------
- Etiketli ASCII SPE (Ortec / Maestro: $DATA:, $MEAS_TIM:, ...)
- GF3 benzeri binary SPE (40 bayt header + float32 little-endian)
- Ortec binary CHN (32 bayt header + uint32 counts, opsiyonel enerji trailer)

Hizli baslangic (WSL / Linux + ROOT)
------------------------------------
  cd /path/to/UniversalSpectrumToolkit
  root -l
  .L universal_spectrum_to_root.C+
  universal_spectrum_help();
  universal_spectrum_to_root("samples/ortec_ascii.spe");
  universal_spectrum_to_root("samples/gf3_like.spe", "", false);
  universal_spectrum_to_root("samples/ortec_demo.chn");

Bir klasordeki tum *.spe ve *.chn dosyalari:
  universal_spectrum_batch("samples");

Bilinmeyen binary icin (sadece bilerek):
  universal_spectrum_to_root("unknown.bin", "", false, true);

ROOT gerekmeden okuyucu testleri
--------------------------------
  cd tests
  g++ -std=c++17 -O2 -o test_readers test_readers.cpp
  ./test_readers

Notlar
------
- Mumkunse etiketli ASCII Ortec SPE tercih edin; live/real time ve
  enerji kalibrasyonu tasir.
- GF3 benzeri binary eslesme yapisal ve bilincli olarak daha katidir.
- Ortec CHN cogu zaman live/real time tasir; trailer'da egim varsa
  enerji kalibrasyonu da kullanilir.
- Ham baytlar izlenebilirlik icin saklanir.

Temel fikir
-----------
ROOT .spe / .chn acamaz -> dosyayi ham bayt olarak oku -> formati
tani -> kanal count dizisini cikar -> TH1D/TTree olarak .root yaz.
