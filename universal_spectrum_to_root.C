#include "SpectrumData.h"
#include "IReader.h"
#include "ByteUtils.h"
#include "AsciiSPEReader.h"
#include "GF3Reader.h"
#include "ReaderRegistry.h"
#include "GenericDetector.h"
#include "RootWriter.h"

#include "AsciiSPEReader.cxx"
#include "GF3Reader.cxx"
#include "ReaderRegistry.cxx"
#include "GenericDetector.cxx"
#include "RootWriter.cxx"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

void universal_spectrum_to_root(
    const char* inputPath,
    const char* outputPath = "",
    bool drawSpectrum = true) {

    using namespace ust;

    if (!inputPath || std::string(inputPath).empty()) {
        std::cerr << "ERROR: input path is empty.\n";
        return;
    }

    try {
        const std::string path(inputPath);
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
            << "File size  : " << rawBytes.size() << " bytes\n";

        ReaderRegistry registry;

        registry.registerReader(
            std::unique_ptr<IReader>(new AsciiSPEReader()));

        registry.registerReader(
            std::unique_ptr<IReader>(new GF3Reader()));

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

        if (registry.chooseWinner(
                results, winner, decisionMessage)) {

            const SpectrumData& data = winner.data;

            std::cout
                << "\nKnown-format decision\n"
                << "---------------------\n"
                << "Reader       : " << data.readerName << '\n'
                << "Format       : " << data.detectedFormat << '\n'
                << "Channels     : " << data.channelCount << '\n'
                << "Numeric type : " << data.numericType << '\n'
                << "Byte order   : " << data.byteOrder << '\n'
                << "Header bytes : " << data.headerBytes << '\n'
                << "Reason       : " << data.decisionReason << '\n';

            const std::string output =
                (outputPath && std::string(outputPath).size() > 0)
                    ? std::string(outputPath)
                    : RootWriter::defaultOutputPath(path);

            RootWriter writer;
            writer.write(data, output, drawSpectrum);

            std::cout
                << "============================================================\n\n";
            return;
        }

        std::cout
            << "\nKnown-reader decision: "
            << decisionMessage << '\n';

        GenericDetector detector;
        const std::vector<GenericCandidate> candidates =
            detector.scan(rawBytes);

        detector.printCandidates(candidates);

        std::cout
            << "============================================================\n\n";

    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
    }
}
