// coincidence_delay_fit.C
// Delay-scan coincidence fits in ROOT.
//
// Fit model (same as reference PNGs):
//   f(x) = Baseline + Amplitude * exp(-0.5*((x-Mean)/Sigma)^2)
//
// IMPORTANT: this macro reads delay_fit_w*.txt  (NOT the .data spectra)
//
// Ubuntu example:
//   1) Put this file next to your data, OR pass absolute paths.
//   2) Data folder must CONTAIN directories like 500ns/, 1000ns/, ...
//      Example tree:
//        /home/you/coinc/500ns/CH1/delay_fit_w500000.txt
//        /home/you/coinc/1000ns/CH1/delay_fit_w1000000.txt
//   3) Run from the directory where this .C lives:
//        root -l -b -q 'coincidence_delay_fit.C("/home/you/coinc","out")'
//
// If it fails, first run with no args to see diagnostics:
//        root -l -b -q coincidence_delay_fit.C

#include <TCanvas.h>
#include <TF1.h>
#include <TGraph.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TSystemFile.h>
#include <TList.h>
#include <TString.h>
#include <TMath.h>

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
  TString windowDir;
  TString channelDir;
  TString windowPs;
};

static void PrintTreeHint(const TString& rootDir)
{
  std::cout << "\nLooking under: " << rootDir << "\n";
  std::cout << "Expected layout:\n"
            << "  <dir>/500ns/CH1/delay_fit_w500000.txt\n"
            << "  <dir>/1000ns/CH1/delay_fit_w1000000.txt\n"
            << "  ...\n";

  void* dir = gSystem->OpenDirectory(rootDir.Data());
  if (!dir) {
    std::cerr << "ERROR: cannot open directory: " << rootDir << "\n";
    std::cerr << "Tip: use an absolute path, e.g. /home/USER/coinc\n";
    return;
  }

  std::cout << "Contents of that directory:\n";
  const char* entry = nullptr;
  int n = 0;
  while ((entry = gSystem->GetDirEntry(dir))) {
    TString name = entry;
    if (name == "." || name == "..") continue;
    FileStat_t st;
    const TString full = rootDir + "/" + name;
    gSystem->GetPathInfo(full, st);
    std::cout << "  " << (R_ISDIR(st.fMode) ? "[DIR] " : "      ") << name << "\n";
    if (++n >= 40) {
      std::cout << "  ...\n";
      break;
    }
  }
  gSystem->FreeDirectory(dir);
}

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

static void CollectJobsInDir(const TString& rootDir, std::vector<FitJob>& jobs)
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
        TString wp = fname;
        wp.ReplaceAll("delay_fit_w", "");
        wp.ReplaceAll(".txt", "");
        job.windowPs = wp;
        jobs.push_back(job);
      }
    }
  }
}

static void CollectJobs(const TString& rootDir, std::vector<FitJob>& jobs)
{
  jobs.clear();
  CollectJobsInDir(rootDir, jobs);
  if (!jobs.empty()) return;

  // If user pointed one level too high/low, search one subdirectory deeper.
  TSystemDirectory top("top", rootDir);
  TList* entries = top.GetListOfFiles();
  if (!entries) return;

  TIter next(entries);
  while (TSystemFile* e = (TSystemFile*)next()) {
    const TString name = e->GetName();
    if (!e->IsDirectory() || name == "." || name == "..") continue;
    CollectJobsInDir(rootDir + "/" + name, jobs);
    if (!jobs.empty()) {
      std::cout << "Note: found data under " << rootDir << "/" << name << "\n";
      break;
    }
  }
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

  auto* fit = new TF1("fit_gaus_baseline",
                      "[0]*TMath::Exp(-0.5*((x-[1])/[2])*((x-[1])/[2]))+[3]",
                      points.front().delay, points.back().delay);
  fit->SetParName(0, "Amplitude");
  fit->SetParName(1, "Mean");
  fit->SetParName(2, "Sigma");
  fit->SetParName(3, "Baseline");
  fit->SetLineColor(kRed);
  fit->SetLineWidth(2);

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
  fit->SetParLimits(0, 0.0, 50.0 * ampSeed);
  fit->SetParLimits(1, points.front().delay - xSpan, points.back().delay + xSpan);
  fit->SetParLimits(2, 1.0, 10.0 * xSpan);
  fit->SetParLimits(3, ymin - 5.0 * ampSeed, ymax + 5.0 * ampSeed);

  const int fitStatus = gr->Fit(fit, "Q");
  const double chi2 = fit->GetChisquare();
  const int ndf = fit->GetNDF();
  const double prob = fit->GetProb();

  const TString outDir = outRoot + "/" + job.windowDir + "/" + job.channelDir;
  gSystem->mkdir(outDir, true);

  auto* c = new TCanvas(Form("c_%s_%s", job.windowDir.Data(), job.channelDir.Data()),
                        "coincidence delay fit", 1200, 800);
  c->SetTicks(1, 1);

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

void coincidence_delay_fit(const char* dataRoot = ".",
                           const char* outRoot = "out")
{
  gROOT->SetBatch(kTRUE);
  gStyle->SetOptFit(1111);
  gStyle->SetOptStat(0);

  TString inDir = dataRoot;
  TString outDir = outRoot;

  // Expand ~ if present
  if (inDir.BeginsWith("~")) {
    inDir = TString(gSystem->HomeDirectory()) + inDir(1, inDir.Length() - 1);
  }
  if (outDir.BeginsWith("~")) {
    outDir = TString(gSystem->HomeDirectory()) + outDir(1, outDir.Length() - 1);
  }

  gSystem->mkdir(outDir, true);

  std::vector<FitJob> jobs;
  CollectJobs(inDir, jobs);
  if (jobs.empty()) {
    std::cerr << "ERROR: no delay_fit_*.txt found.\n";
    PrintTreeHint(inDir);
    std::cerr << "\nRemember: fit uses .txt files, not .data spectra.\n";
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
