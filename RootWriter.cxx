#include "RootWriter.h"

#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1D.h>
#include <TNamed.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

namespace ust {

std::string RootWriter::defaultOutputPath(
    const std::string& inputPath) {

    const std::size_t slash =
        inputPath.find_last_of("/\\");

    const std::size_t dot =
        inputPath.find_last_of('.');

    const bool validDot =
        dot != std::string::npos &&
        (slash == std::string::npos || dot > slash);

    if (validDot) {
        return inputPath.substr(0, dot) + ".root";
    }

    return inputPath + ".root";
}

bool RootWriter::write(
    const SpectrumData& data,
    const std::string& outputPath,
    bool drawSpectrum,
    bool logY) const {

    if (!data.valid || data.counts.empty()) {
        std::cerr
            << "ERROR: no valid spectrum is available for writing.\n";
        return false;
    }

    std::unique_ptr<TFile> output(
        TFile::Open(outputPath.c_str(), "RECREATE"));

    if (!output || output->IsZombie()) {
        std::cerr
            << "ERROR: cannot create ROOT file: "
            << outputPath << '\n';
        return false;
    }

    const int bins = static_cast<int>(data.counts.size());
    const double xMin =
        static_cast<double>(data.firstChannel);
    const double xMax =
        static_cast<double>(data.firstChannel + bins);

    TH1D histogram(
        "hSpectrumChannel",
        "Converted spectrum;Channel;Counts",
        bins,
        xMin,
        xMax);

    histogram.SetDirectory(output.get());
    histogram.SetStats(kFALSE);

    for (int i = 0; i < bins; ++i) {
        histogram.SetBinContent(
            i + 1,
            data.counts[static_cast<std::size_t>(i)]);
    }

    TH1D* energyHistogram = nullptr;

    if (data.hasEnergyCalibration &&
        std::abs(data.energyB) > 0.0) {

        const double e0 =
            data.energyA +
            data.energyB * xMin +
            data.energyC * xMin * xMin;

        const double e1 =
            data.energyA +
            data.energyB * xMax +
            data.energyC * xMax * xMax;

        const double eMin = std::min(e0, e1);
        const double eMax = std::max(e0, e1);

        energyHistogram = new TH1D(
            "hSpectrumEnergy",
            "Converted spectrum;Energy;Counts",
            bins,
            eMin,
            eMax);

        energyHistogram->SetDirectory(output.get());
        energyHistogram->SetStats(kFALSE);

        for (int i = 0; i < bins; ++i) {
            const double channel =
                xMin + static_cast<double>(i) + 0.5;
            const double energy =
                data.energyA +
                data.energyB * channel +
                data.energyC * channel * channel;
            const int energyBin =
                energyHistogram->FindBin(energy);
            energyHistogram->AddBinContent(
                energyBin,
                data.counts[static_cast<std::size_t>(i)]);
        }
    }

    int channel = 0;
    double counts = 0.0;
    double energy = 0.0;

    TTree spectrumTree(
        "spectrum",
        "Channel-by-channel spectrum");

    spectrumTree.Branch("channel", &channel, "channel/I");
    spectrumTree.Branch("counts", &counts, "counts/D");
    spectrumTree.Branch("energy", &energy, "energy/D");

    for (int i = 0; i < bins; ++i) {
        channel = static_cast<int>(data.firstChannel) + i;
        counts = data.counts[static_cast<std::size_t>(i)];

        if (data.hasEnergyCalibration) {
            const double ch = static_cast<double>(channel);
            energy =
                data.energyA +
                data.energyB * ch +
                data.energyC * ch * ch;
        } else {
            energy = static_cast<double>(channel);
        }

        spectrumTree.Fill();
    }

    std::string sourceFile = data.sourceFile;
    std::string extension = data.extension;
    std::string detectedFormat = data.detectedFormat;
    std::string readerName = data.readerName;
    std::string numericType = data.numericType;
    std::string byteOrder = data.byteOrder;
    std::string decisionReason = data.decisionReason;
    std::string specId = data.specId;
    std::string dateMeasured = data.dateMeasured;

    ULong64_t fileSize =
        static_cast<ULong64_t>(data.fileSize);
    ULong64_t headerBytes =
        static_cast<ULong64_t>(data.headerBytes);
    ULong64_t channelCount =
        static_cast<ULong64_t>(data.channelCount);
    ULong64_t valueBytes =
        static_cast<ULong64_t>(data.bytesPerValue);
    ULong64_t firstChannel =
        static_cast<ULong64_t>(data.firstChannel);
    ULong64_t lastChannel =
        static_cast<ULong64_t>(data.lastChannel);

    double readerScore = data.readerScore;
    double liveTime = data.liveTime;
    double realTime = data.realTime;
    double energyA = data.energyA;
    double energyB = data.energyB;
    double energyC = data.energyC;
    int hasEnergyCalibration =
        data.hasEnergyCalibration ? 1 : 0;

    TTree metadata(
        "conversion_metadata",
        "Universal converter metadata");

    metadata.Branch("source_file", &sourceFile);
    metadata.Branch("extension", &extension);
    metadata.Branch("detected_format", &detectedFormat);
    metadata.Branch("reader_name", &readerName);
    metadata.Branch("numeric_type", &numericType);
    metadata.Branch("byte_order", &byteOrder);
    metadata.Branch("decision_reason", &decisionReason);
    metadata.Branch("spec_id", &specId);
    metadata.Branch("date_measured", &dateMeasured);
    metadata.Branch("file_size", &fileSize, "file_size/l");
    metadata.Branch("header_bytes", &headerBytes, "header_bytes/l");
    metadata.Branch("channel_count", &channelCount, "channel_count/l");
    metadata.Branch("bytes_per_value", &valueBytes, "bytes_per_value/l");
    metadata.Branch("first_channel", &firstChannel, "first_channel/l");
    metadata.Branch("last_channel", &lastChannel, "last_channel/l");
    metadata.Branch("reader_score", &readerScore, "reader_score/D");
    metadata.Branch("live_time", &liveTime, "live_time/D");
    metadata.Branch("real_time", &realTime, "real_time/D");
    metadata.Branch("energy_a", &energyA, "energy_a/D");
    metadata.Branch("energy_b", &energyB, "energy_b/D");
    metadata.Branch("energy_c", &energyC, "energy_c/D");
    metadata.Branch(
        "has_energy_calibration",
        &hasEnergyCalibration,
        "has_energy_calibration/I");
    metadata.Fill();

    // Compact, ROOT 6.x friendly raw-byte storage:
    // one tree entry holding the full byte vector.
    TDirectory* rawDirectory = output->mkdir("raw");
    rawDirectory->cd();

    std::vector<unsigned char> rawBytes = data.rawBytes;

    TTree rawTree(
        "raw_bytes",
        "Exact original input bytes (single vector entry)");

    rawTree.Branch("bytes", &rawBytes);
    rawTree.Fill();
    rawTree.Write();

    TNamed rawNote(
        "raw_bytes_note",
        "Exact original bytes stored in raw/raw_bytes.bytes");
    rawNote.Write();

    output->cd();

    if (!data.specId.empty()) {
        TNamed("spec_id", data.specId.c_str()).Write();
    }
    if (!data.dateMeasured.empty()) {
        TNamed("date_measured", data.dateMeasured.c_str()).Write();
    }

    histogram.Write();
    if (energyHistogram) {
        energyHistogram->Write();
    }
    spectrumTree.Write();
    metadata.Write();

    output->Write();
    output->Close();

    std::cout
        << "\nROOT output written successfully:\n"
        << "  " << outputPath << "\n"
        << "Objects:\n"
        << "  TH1D  hSpectrumChannel\n";

    if (data.hasEnergyCalibration &&
        std::abs(data.energyB) > 0.0) {
        std::cout << "  TH1D  hSpectrumEnergy\n";
    }

    std::cout
        << "  TTree spectrum\n"
        << "  TTree conversion_metadata\n"
        << "  raw/raw_bytes\n";

    if (drawSpectrum) {
        auto* canvas = new TCanvas(
            "cUniversalSpectrum",
            "Universal Spectrum Toolkit",
            1300,
            760);

        auto* displayHistogram =
            static_cast<TH1D*>(
                histogram.Clone("hUniversalSpectrumDisplay"));

        displayHistogram->SetDirectory(nullptr);
        displayHistogram->SetStats(kFALSE);

        canvas->SetGrid();

        if (logY) {
            displayHistogram->SetMinimum(0.5);
            canvas->SetLogy(kTRUE);
        } else {
            displayHistogram->SetMinimum(0.0);
            canvas->SetLogy(kFALSE);
        }

        displayHistogram->Draw("hist");
        canvas->Modified();
        canvas->Update();
    }

    return true;
}

} // namespace ust
