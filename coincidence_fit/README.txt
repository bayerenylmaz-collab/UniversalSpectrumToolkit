Coincidence delay-scan fits (ROOT)
=================================

What this is
------------
The archive under `coincidence_data/` contains delay-scan coincidence
curves already extracted as:

  <window>ns/CH<n>/delay_fit_w<window_ps>.txt

plus reference PNG fits produced elsewhere (not in ROOT).

This folder reproduces those fits in ROOT with the same model:

  f(x) = Baseline + Amplitude * exp(-0.5 * ((x - Mean) / Sigma)^2)

Data layout (extracted archive)
-------------------------------
- Windows: 500ns, 1000ns, 2000ns, 4000ns, 8000ns, 10000ns
- Channels: CH1, CH2, CH3
- Per channel:
  - `delay_fit_w*.txt`  — delay(ps) vs coincidence count (100 points)
  - `delay_fit_w*.png`  — reference fit image
  - `*_coinc_gf3.data` / `*_coinc_hdtv.data` — coincidence spectra at
    selected delays (not required for the delay-curve fit itself)

Requirements
------------
ROOT 6.x (tested with 6.32.08). Example:

  source /opt/root/bin/thisroot.sh

Run
---
From the repository root:

  root -l -b -q 'coincidence_fit/coincidence_delay_fit.C("coincidence_data/extracted","coincidence_fit/out")'

Outputs
-------
`coincidence_fit/out/<window>/<CH>/`
  - `delay_fit_w*.png`       reproduced plot
  - `delay_fit_params.txt`   Amplitude, Mean, Sigma, Baseline, chi2/ndf

`coincidence_fit/out/fit_summary.csv` — all 18 fits in one table.

Notes
-----
- Reference PNGs intentionally use a Gaussian+baseline even when the
  curve is top-hat shaped; chi2 is therefore large. That is expected.
- For 500ns/CH1 the ROOT result matches the reference parameters:
  Amplitude ~ 3.438e4, Mean ~ 1.627e5, Sigma ~ 3.731e5,
  Baseline ~ 3.826e4, chi2/ndf ~ 5.63e8/96.
- Wider windows make the Gaussian poorly constrained; parameters can
  drift while chi2 stays similar. Seeds/limits in the macro keep the
  fit numerically stable.
