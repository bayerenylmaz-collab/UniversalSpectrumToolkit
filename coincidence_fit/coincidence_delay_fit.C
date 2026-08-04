// coincidence_delay_fit.C
 // Reproduce delay-scan coincidence fits in ROOT.
 //
 // Input layout (from the provided archive):
 //   <window>ns/CH<n>/delay_fit_w<window_ps>.txt
 //   columns: delay(ps)  coincidence
 //
 // Fit model (same as reference PNGs):
 //   f(x) = Baseline + Amplitude * exp(-0.5 * ((x-Mean)/Sigma)^2)
 //
 // Usage:
 //   source /opt/root/bin/thisroot.sh   # if needed
 //   root -l -b -q 'coincidence_delay_fit.C("path/to/extracted")'
 //
 // Outputs under coincidence_fit/out/<window>/<CH>/ :
 //   delay_fit_w*.png, delay_fit_params.txt
 // plus a global summary CSV.

#include <TCanvas.h>
#include <TF1.h>
#include <TGraph.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TSystemFile.h>
#include <TList.h>
#include <TString.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

struct DelayPoint {
  double delay = 0.0;
  double coinc = 0.0;
};

struct FitJob {
  TString txtPath;
  TString windowDir;   // e.g. 500ns
  TString channelDir;  // e.g. CH1
  TString windowPs;    // e.g. 500000
};

static bool ReadDelayFitTxt(const TString& path, std::vector<DelayPoint>& points)
{
  points.clear();
  std::ifstream in(path.Data());
  if (!in) {
    std::cerr << "ERROR: cannot open " << path << "\n";
    return false;
  }

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream ss(line);
    DelayPoint p;
    if (!(ss >> p.delay >> p.coinc)) continue;
    points.push_back(p);
  }
  return !points.empty();
}

static void CollectJobs(const TString& rootDir, std::vector<FitJob>& jobs)
{
  TSystemDirectory top("top", rootDir);
  TList* windows = top.GetListOfFiles();
  if (!windows) return;

  TIter nextWin(windows);
  while (TSystemFile* win = (TSystemFile*)nextWin()) {
    const TString wname = win->GetName();
    if (!win->IsDirectory() || wname == "." || wname == "..") continue;
    if (!wname.EndsWith("ns")) continue;

    const TString winPath = rootDir + "/" + wname;
    TSystemDirectory winDir(wname, winPath);
    TList* chans = winDir.GetListOfFiles();
    if (!chans) continue;

    TIter nextCh(chans);
    while (TSystemFile* ch = (TSystemFile*)nextCh()) {
      const TString cname = ch->GetName();
      if (!ch->IsDirectory() || cname == "." || cname == "..") continue;
      if (!cname.BeginsWith("CH")) continue;

      const TString chPath = winPath + "/" + cname;
      TSystemDirectory chDir(cname, chPath);
      TList* files = chDir.GetListOfFiles();
      if (!files) continue;

      TIter nextF(files);
      while (TSystemFile* f = (TSystemFile*)nextF()) {
        if (f->IsDirectory()) continue;
        const TString fname = f->GetName();
        if (!fname.BeginsWith("delay_fit_w") || !fname.EndsWith(".txt")) continue;

        FitJob job;
        job.txtPath = chPath + "/" + fname;
        job.windowDir = wname;
        job.channelDir = cname;
        // delay_fit_w500000.txt -> 500000
        TString wp = fname;
        wp.ReplaceAll("delay_fit_w", "");
        wp.ReplaceAll(".txt", "");
        job.windowPs = wp;
        jobs.push_back(job);
      }
    }
  }

  std::sort(jobs.begin(), jobs.end(), [](const FitJob& a, const FitJob& b) {
    if (a.windowDir != b.windowDir) return a.windowDir < b.windowDir;
    return a.channelDir < b.channelDir;
  });
}

static bool FitOne(const FitJob& job, const TString& outRoot, std::ostream& summary)
{
  std::vector<DelayPoint> points;
  if (!ReadDelayFitTxt(job.txtPath, points)) return false;

  const int n = (int)points.size();
  auto* gr = new TGraph(n);
  gr->SetName(Form("g_%s_%s", job.windowDir.Data(), job.channelDir.Data()));
  gr->SetTitle(Form("Window %s ps;Delay (ps);Coincidence Count", job.windowPs.Data()));
  gr->SetMarkerStyle(20);
  gr->SetMarkerSize(0.7);
  gr->SetMarkerColor(kBlack);
  gr->SetLineColor(kBlack);

  double ymin = points[0].coinc, ymax = points[0].coinc;
  double ySum = 0.0;
  for (int i = 0; i < n; ++i) {
    gr->SetPoint(i, points[i].delay, points[i].coinc);
    ymin = std::min(ymin, points[i].coinc);
    ymax = std::max(ymax, points[i].coinc);
    ySum += points[i].coinc;
  }
  const double yMean = ySum / n;

  // Same model as reference plots: Baseline + Amplitude * Gaus(x; Mean, Sigma)
  auto* fit = new TF1("fit_gaus_baseline",
                      "[0]*TMath::Exp(-0.5*((x-[1])/[2])*((x-[1])/[2]))+[3]",
                      points.front().delay, points.back().delay);
  fit->SetParName(0, "Amplitude");
  fit->SetParName(1, "Mean");
  fit->SetParName(2, "Sigma");
  fit->SetParName(3, "Baseline");
  fit->SetLineColor(kRed);
  fit->SetLineWidth(2);

  // Seeds from edge baseline + half-maximum width (stable for top-hat curves)
  const int kEdge = std::max(3, n / 10);
  double edgeSum = 0.0;
  for (int i = 0; i < kEdge; ++i) edgeSum += points[i].coinc;
  for (int i = n - kEdge; i < n; ++i) edgeSum += points[i].coinc;
  const double baselineSeed = edgeSum / (2.0 * kEdge);
  const double ampSeed = std::max(1.0, ymax - baselineSeed);

  const double halfMax = baselineSeed + 0.5 * ampSeed;
  int iLeft = 0, iRight = n - 1;
  for (int i = 0; i < n; ++i) {
    if (points[i].coinc >= halfMax) { iLeft = i; break; }
  }
  for (int i = n - 1; i >= 0; --i) {
    if (points[i].coinc >= halfMax) { iRight = i; break; }
  }
  const double meanSeed = 0.5 * (points[iLeft].delay + points[iRight].delay);
  const double fwhm = std::max(1.0, points[iRight].delay - points[iLeft].delay);
  const double sigmaSeed = std::max(1.0, fwhm / 2.355);
  const double xSpan = std::max(1.0, points.back().delay - points.front().delay);

  fit->SetParameters(ampSeed, meanSeed, sigmaSeed, baselineSeed);
  fit->SetParLimits(0, 0.0, 50.0 * ampSeed);           // Amplitude >= 0
  fit->SetParLimits(1, points.front().delay - xSpan, points.back().delay + xSpan);
  fit->SetParLimits(2, 1.0, 10.0 * xSpan);              // Sigma > 0
  fit->SetParLimits(3, ymin - 5.0 * ampSeed, ymax + 5.0 * ampSeed);

  // Equal weights (ROOT TGraph default) — matches reference chi2 values
  const int fitStatus = gr->Fit(fit, "Q");

  const double chi2 = fit->GetChisquare();
  const int ndf = fit->GetNDF();
  const double prob = fit->GetProb();

  const TString outDir = outRoot + "/" + job.windowDir + "/" + job.channelDir;
  gSystem->mkdir(outDir, true);

  // Canvas styled close to the reference PNGs
  auto* c = new TCanvas(Form("c_%s_%s", job.windowDir.Data(), job.channelDir.Data()),
                        "coincidence delay fit", 1200, 800);
  c->SetGrid(0, 0);
  c->SetTicks(1, 1);

  // Include fit peak in the drawn Y range (Gaussian can overshoot the plateau)
  const double fitPeak = fit->GetParameter(3) + fit->GetParameter(0);
  const double yLo = std::min(ymin, fit->Eval(points.front().delay));
  const double yHi = std::max(ymax, fitPeak);
  const double yPad = 0.08 * (yHi - yLo + 1.0);
  gr->GetYaxis()->SetRangeUser(yLo - yPad, yHi + yPad);
  gr->Draw("APL");
  fit->Draw("SAME");

  gStyle->SetOptFit(1111);
  gStyle->SetOptStat(0);

  const TString pngPath = outDir + "/delay_fit_w" + job.windowPs + ".png";
  c->SaveAs(pngPath);

  const TString parPath = outDir + "/delay_fit_params.txt";
  std::ofstream pout(parPath.Data());
  pout << "# source " << job.txtPath << "\n";
  pout << "# model Baseline + Amplitude * exp(-0.5*((x-Mean)/Sigma)^2)\n";
  pout << "fit_status\t" << fitStatus << "\n";
  pout << "chi2\t" << chi2 << "\n";
  pout << "ndf\t" << ndf << "\n";
  pout << "prob\t" << prob << "\n";
  pout << "Amplitude\t" << fit->GetParameter(0) << "\t" << fit->GetParError(0) << "\n";
  pout << "Mean\t" << fit->GetParameter(1) << "\t" << fit->GetParError(1) << "\n";
  pout << "Sigma\t" << fit->GetParameter(2) << "\t" << fit->GetParError(2) << "\n";
  pout << "Baseline\t" << fit->GetParameter(3) << "\t" << fit->GetParError(3) << "\n";
  pout << "n_points\t" << n << "\n";
  pout << "y_mean\t" << yMean << "\n";
  pout.close();

  summary << job.windowDir << "," << job.channelDir << "," << job.windowPs << ","
          << n << "," << fitStatus << "," << chi2 << "," << ndf << "," << prob << ","
          << fit->GetParameter(0) << "," << fit->GetParError(0) << ","
          << fit->GetParameter(1) << "," << fit->GetParError(1) << ","
          << fit->GetParameter(2) << "," << fit->GetParError(2) << ","
          << fit->GetParameter(3) << "," << fit->GetParError(3) << "\n";

  std::cout << job.windowDir << "/" << job.channelDir
            << "  A=" << fit->GetParameter(0)
            << "  Mean=" << fit->GetParameter(1)
            << "  Sigma=" << fit->GetParameter(2)
            << "  B=" << fit->GetParameter(3)
            << "  chi2/ndf=" << chi2 << "/" << ndf
            << "  -> " << pngPath << "\n";

  delete c;
  delete gr;
  delete fit;
  return true;
}

void coincidence_delay_fit(const char* dataRoot =
                               "coincidence_data/extracted",
                           const char* outRoot = "coincidence_fit/out")
{
  gROOT->SetBatch(kTRUE);
  gStyle->SetOptFit(1111);
  gStyle->SetOptStat(0);
  gStyle->SetTitleFontSize(0.04);

  TString inDir = dataRoot;
  TString outDir = outRoot;
  gSystem->mkdir(outDir, true);

  std::vector<FitJob> jobs;
  CollectJobs(inDir, jobs);
  if (jobs.empty()) {
    std::cerr << "ERROR: no delay_fit_*.txt under " << inDir << "\n";
    return;
  }

  std::cout << "Found " << jobs.size() << " delay-fit curves under " << inDir << "\n";

  const TString summaryPath = outDir + "/fit_summary.csv";
  std::ofstream summary(summaryPath.Data());
  summary << "window,channel,window_ps,n_points,fit_status,chi2,ndf,prob,"
             "Amplitude,Amplitude_err,Mean,Mean_err,Sigma,Sigma_err,Baseline,Baseline_err\n";

  int ok = 0;
  for (const auto& job : jobs) {
    if (FitOne(job, outDir, summary)) ++ok;
  }
  summary.close();

  std::cout << "Done: " << ok << "/" << jobs.size()
            << " fits. Summary: " << summaryPath << "\n";
}
