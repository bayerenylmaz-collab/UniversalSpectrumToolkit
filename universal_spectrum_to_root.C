#include "SpectrumData.h"
#include "IReader.h"
#include "ByteUtils.h"
#include "AsciiSPEReader.h"
#include "GF3Reader.h"
#include "OrtecCHNReader.h"
#include "ReaderRegistry.h"
#include "GenericDetector.h"
#include "RootWriter.h"

#include "AsciiSPEReader.cxx"
#include "GF3Reader.cxx"
#include "OrtecCHNReader.cxx"
#include "ReaderRegistry.cxx"
#include "GenericDetector.cxx"
#include "RootWriter.cxx"

#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TSystemFile.h>
#include <TList.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

bool convertOne(
    const std::string& path,
    const std::string& outputPath,
    bool drawSpectrum,
    bool forceGeneric) {

    using namespace ust;

    const std::string extension = extensionOf(path);
    const std::vector<unsigned char> rawBytes =
        readAllBytes(path);

    std::cout
        << "\n============================================================\n"
        << " UNIVERSAL SPECTRUM TOOLKIT\n"
        << "============================================================\n"
        << "Input file : " << path << '\n'
        << "Extension  : "
        << (extension.empty() ? "[none]" : extension) << '\n'
        << "File size  : " << rawBytes.size() << " bytes\n"
        << "Force generic : "
        << (forceGeneric ? "yes" : "no") << '\n';

    ReaderRegistry registry;
    registry.registerReader(
        std::unique_ptr<IReader>(new AsciiSPEReader()));
    registry.registerReader(
        std::unique_ptr<IReader>(new GF3Reader()));
    registry.registerReader(
        std::unique_ptr<IReader>(new OrtecCHNReader()));

    const std::vector<ReaderResult> results =
        registry.runReaders(path, extension, rawBytes);

    if (!results.empty()) {
        std::cout << "\nRegistered readers for this extension:\n";
        for (const ReaderResult& result : results) {
            std::cout
                << "  - " << result.readerName << ": "
                << (result.matched ? "MATCH" : "no match")
                << " | " << result.reason << '\n';
        }
    } else {
        std::cout
            << "\nNo readers are registered for extension "
            << (extension.empty() ? "[none]" : extension)
            << ".\n";
    }

    ReaderResult winner;
    std::string decisionMessage;

    if (registry.chooseWinner(results, winner, decisionMessage)) {
        const SpectrumData& data = winner.data;

        std::cout
            << "\nKnown-format decision\n"
            << "---------------------\n"
            << "Reader       : " << data.readerName << '\n'
            << "Format       : " << data.detectedFormat << '\n'
            << "Channels     : " << data.channelCount << '\n'
            << "Numeric type : " << data.numericType << '\n'
            << "Byte order   : " << data.byteOrder << '\n'
            << "Live time    : " << data.liveTime << '\n'
            << "Real time    : " << data.realTime << '\n'
            << "Energy cal   : "
            << (data.hasEnergyCalibration ? "yes" : "no") << '\n'
            << "Header bytes : " << data.headerBytes << '\n'
            << "Reason       : " << data.decisionReason << '\n';

        const std::string output =
            !outputPath.empty()
                ? outputPath
                : RootWriter::defaultOutputPath(path);

        RootWriter writer;
        const bool ok =
            writer.write(data, output, drawSpectrum);

        std::cout
            << "============================================================\n\n";
        return ok;
    }

    std::cout
        << "\nKnown-reader decision: "
        << decisionMessage << '\n';

    GenericDetector detector;
    const std::vector<GenericCandidate> candidates =
        detector.scan(rawBytes);

    if (!forceGeneric) {
        detector.printCandidates(candidates);
        std::cout
            << "============================================================\n\n";
        return false;
    }

    SpectrumData forced;
    std::string forceReason;

    if (!detector.tryForceConvert(
            path, rawBytes, forced, forceReason)) {
        std::cout
            << "\nForced generic conversion refused:\n  "
            << forceReason << '\n';
        detector.printCandidates(candidates);
        std::cout
            << "============================================================\n\n";
        return false;
    }

    std::cout
        << "\nForced-generic decision\n"
        << "-----------------------\n"
        << "Reader       : " << forced.readerName << '\n'
        << "Format       : " << forced.detectedFormat << '\n'
        << "Channels     : " << forced.channelCount << '\n'
        << "Numeric type : " << forced.numericType << '\n'
        << "Byte order   : " << forced.byteOrder << '\n'
        << "Header bytes : " << forced.headerBytes << '\n'
        << "Reason       : " << forced.decisionReason << '\n';

    const std::string output =
        !outputPath.empty()
            ? outputPath
            : RootWriter::defaultOutputPath(path);

    RootWriter writer;
    const bool ok =
        writer.write(forced, output, drawSpectrum);

    std::cout
        << "============================================================\n\n";
    return ok;
}

} // namespace

// Single-file conversion.
// Example:
//   universal_spectrum_to_root("sample.spe");
//   universal_spectrum_to_root("sample.spe", "out.root", false);
//   universal_spectrum_to_root("unknown.spe", "", false, true);
void universal_spectrum_to_root(
    const char* inputPath,
    const char* outputPath = "",
    bool drawSpectrum = true,
    bool forceGeneric = false) {

    if (!inputPath || std::string(inputPath).empty()) {
        std::cerr << "ERROR: input path is empty.\n";
        return;
    }

    try {
        convertOne(
            std::string(inputPath),
            outputPath ? std::string(outputPath) : std::string(),
            drawSpectrum,
            forceGeneric);
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
    }
}

// Convert every *.spe / *.chn under a directory (non-recursive).
// Drawing is off by default for batch use.
void universal_spectrum_batch(
    const char* directoryPath,
    bool drawSpectrum = false,
    bool forceGeneric = false) {

    if (!directoryPath ||
        std::string(directoryPath).empty()) {
        std::cerr << "ERROR: directory path is empty.\n";
        return;
    }

    TSystemDirectory directory(
        directoryPath, directoryPath);

    TList* files = directory.GetListOfFiles();
    if (!files) {
        std::cerr
            << "ERROR: cannot list directory: "
            << directoryPath << '\n';
        return;
    }

    int converted = 0;
    int failed = 0;

    TIter next(files);
    while (TObject* object = next()) {
        auto* file = dynamic_cast<TSystemFile*>(object);
        if (!file || file->IsDirectory()) {
            continue;
        }

        const std::string name = file->GetName();
        const std::string extension = ust::extensionOf(name);
        if (extension != ".spe" && extension != ".chn") {
            continue;
        }

        const std::string fullPath =
            std::string(directoryPath) + "/" + name;

        try {
            const bool ok = convertOne(
                fullPath, "", drawSpectrum, forceGeneric);
            if (ok) {
                ++converted;
            } else {
                ++failed;
            }
        } catch (const std::exception& error) {
            std::cerr
                << "ERROR converting " << fullPath
                << ": " << error.what() << '\n';
            ++failed;
        }
    }

    std::cout
        << "Batch finished. Converted=" << converted
        << " Failed=" << failed << '\n';
}

void universal_spectrum_help() {
    std::cout
        << "\nUniversal Spectrum Toolkit\n"
        << "--------------------------\n"
        << "Easy menu (recommended for beginners):\n"
        << "  ./ust_menu.sh\n"
        << "  (see also BASLA.txt)\n\n"
        << "Load in ROOT:\n"
        << "  .L universal_spectrum_to_root.C+\n\n"
        << "Single file:\n"
        << "  universal_spectrum_to_root(\"file.spe\");\n"
        << "  universal_spectrum_to_root(\"file.chn\");\n"
        << "  universal_spectrum_to_root(\"file.spe\", \"out.root\", false);\n"
        << "  universal_spectrum_to_root(\"file.spe\", \"\", false, true);  // force generic\n\n"
        << "Batch directory (*.spe, *.chn):\n"
        << "  universal_spectrum_batch(\"path/to/dir\");\n"
        << "  universal_spectrum_batch(\"path/to/dir\", false, true);\n\n"
        << "Known readers:\n"
        << "  - Tagged ASCII SPE (Ortec/Maestro $DATA:)\n"
        << "  - GF3-like binary SPE (40-byte header + float32 LE)\n"
        << "  - Ortec binary CHN (32-byte header + uint32 counts)\n"
        << "Unknown formats are scanned and reported; they are not\n"
        << "silently converted unless forceGeneric=true.\n\n";
}
