UNIVERSAL SPECTRUM TOOLKIT - MODULAR V1

Copy the whole folder to WSL:
  cp -r /mnt/c/Users/PC/Downloads/UniversalSpectrumToolkit ~/ROOT_TARLA/

Run:
  cd ~/ROOT_TARLA/UniversalSpectrumToolkit
  root -l
  .L universal_spectrum_to_root.C+
  universal_spectrum_to_root("file.spe");

Current readers:
- Tagged ASCII SPE ($DATA:)
- GF3-like binary SPE (40-byte header + float32 little-endian)

Unknown formats are only scanned and reported; they are not silently converted.
Raw bytes are stored in a ROOT 6.40-compatible TTree at raw/raw_bytes.
