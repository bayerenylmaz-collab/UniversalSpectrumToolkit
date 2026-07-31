#include "RootWriter.h"

#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1D.h>
#include <TNamed.h>
#include <TTree.h>

#include <iostream>
#include <memory>

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
    bool drawSpectrum) const {

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

    const int bins =
        static_cast<int>(data.counts.size());

    TH1D histogram(
        "hSpectrumChannel",
        "Converted spectrum;Channel;Counts",
        bins,
        0.0,
        static_cast<double>(bins));

    histogram.SetDirectory(output.get());
    histogram.SetStats(false);

    for (int i = 0; i < bins; ++i) {
        histogram.SetBinContent(
            i + 1,
            data.counts[static_cast<std::size_t>(i)]);
    }

    int channel = 0;
    double counts = 0.0;

    TTree spectrumTree(
        "spectrum",
        "Channel-by-channel spectrum");

    spectrumTree.Branch(
        "channel", &channel, "channel/I");

    spectrumTree.Branch(
        "counts", &counts, "counts/D");

    for (int i = 0; i < bins; ++i) {
        channel = i;
        counts =
            data.counts[static_cast<std::size_t>(i)];
        spectrumTree.Fill();
    }

    std::string sourceFile = data.sourceFile;
    std::string extension = data.extension;
    std::string detectedFormat = data.detectedFormat;
    std::string readerName = data.readerName;
    std::string numericType = data.numericType;
    std::string byteOrder = data.byteOrder;
    std::string decisionReason = data.decisionReason;

    ULong64_t fileSize =
        static_cast<ULong64_t>(data.fileSize);

    ULong64_t headerBytes =
        static_cast<ULong64_t>(data.headerBytes);

    ULong64_t channelCount =
        static_cast<ULong64_t>(data.channelCount);

    ULong64_t valueBytes =
        static_cast<ULong64_t>(data.bytesPerValue);

    double readerScore = data.readerScore;

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
    metadata.Branch("file_size", &fileSize);
    metadata.Branch("header_bytes", &headerBytes);
    metadata.Branch("channel_count", &channelCount);
    metadata.Branch("bytes_per_value", &valueBytes);
    metadata.Branch("reader_score", &readerScore);
    metadata.Fill();

    TDirectory* rawDirectory =
        output->mkdir("raw");

    rawDirectory->cd();

    ULong64_t byteIndex = 0;
    UChar_t byteValue = 0;

    TTree rawTree(
        "raw_bytes",
        "Exact original input bytes");

    rawTree.Branch(
        "index", &byteIndex, "index/l");

    rawTree.Branch(
        "value", &byteValue, "value/b");

    for (std::size_t i = 0;
         i < data.rawBytes.size();
         ++i) {

        byteIndex = static_cast<ULong64_t>(i);
        byteValue =
            static_cast<UChar_t>(data.rawBytes[i]);

        rawTree.Fill();
    }

    rawTree.Write();

    TNamed rawNote(
        "raw_bytes_note",
        "Exact original bytes stored in raw/raw_bytes.");

    rawNote.Write();

    output->cd();

    histogram.Write();
    spectrumTree.Write();
    metadata.Write();

    output->Write();
    output->Close();

    std::cout
        << "\nROOT output written successfully:\n"
        << "  " << outputPath << "\n"
        << "Objects:\n"
        << "  TH1D  hSpectrumChannel\n"
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
                histogram.Clone(
                    "hUniversalSpectrumDisplay"));

        displayHistogram->SetDirectory(nullptr);
        displayHistogram->SetMinimum(0.5);
        displayHistogram->SetStats(false);

        canvas->SetGrid();
        canvas->SetLogy();

        displayHistogram->Draw("hist");

        canvas->Modified();
        canvas->Update();
    }

    return true;
}

} // namespace ust
