UNIVERSAL SPECTRUM TOOLKIT
==========================

CERN / lab helper for converting MCA spectrum files that ROOT cannot
open natively (especially .spe) into analysis-ready ROOT files.

What it does
------------
1. Detects known SPE layouts with registered readers
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


Quick start (WSL / Linux with ROOT)
-----------------------------------
  cd /path/to/UniversalSpectrumToolkit
  root -l
  .L universal_spectrum_to_root.C+
  universal_spectrum_help();
  universal_spectrum_to_root("samples/ortec_ascii.spe");
  universal_spectrum_to_root("samples/gf3_like.spe", "", false);

Batch convert every *.spe in a directory:
  universal_spectrum_batch("samples");

Force the best generic binary layout only when you accept the risk:
  universal_spectrum_to_root("unknown.spe", "", false, true);


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
- Raw bytes are preserved for provenance / re-analysis.
