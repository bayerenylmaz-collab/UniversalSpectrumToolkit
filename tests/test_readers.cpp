// Standalone reader tests (no ROOT required).
// Build from this directory:
//   g++ -std=c++17 -O2 -o test_readers test_readers.cpp
//   ./test_readers

#include "../SpectrumData.h"
#include "../ByteUtils.h"
#include "../AsciiSPEReader.h"
#include "../GF3Reader.h"
#include "../ReaderRegistry.h"
#include "../GenericDetector.h"

#include "../AsciiSPEReader.cxx"
#include "../GF3Reader.cxx"
#include "../ReaderRegistry.cxx"
#include "../GenericDetector.cxx"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    } else {
        std::cout << "PASS: " << message << '\n';
    }
}

} // namespace

int main() {
    using namespace ust;

    const std::string asciiPath = "../samples/ortec_ascii.spe";
    const std::string gf3Path = "../samples/gf3_like.spe";

    const auto asciiBytes = readAllBytes(asciiPath);
    const auto gf3Bytes = readAllBytes(gf3Path);

    AsciiSPEReader asciiReader;
    GF3Reader gf3Reader;

    const ReaderResult asciiOnAscii =
        asciiReader.tryRead(asciiPath, asciiBytes);
    expect(asciiOnAscii.matched, "ASCII reader matches Ortec SPE");
    expect(asciiOnAscii.data.channelCount == 256,
           "ASCII reader channel count is 256");
    expect(asciiOnAscii.data.liveTime == 100.0,
           "ASCII reader live time parsed");
    expect(asciiOnAscii.data.realTime == 101.5,
           "ASCII reader real time parsed");
    expect(asciiOnAscii.data.hasEnergyCalibration,
           "ASCII reader energy calibration parsed");
    expect(std::abs(asciiOnAscii.data.energyB - 1.25) < 1e-9,
           "ASCII reader energy slope is 1.25");
    expect(asciiOnAscii.data.counts[21] == 9200.0,
           "ASCII reader peak channel value");

    const ReaderResult gf3OnAscii =
        gf3Reader.tryRead(asciiPath, asciiBytes);
    expect(!gf3OnAscii.matched,
           "GF3 reader rejects text SPE");

    const ReaderResult gf3OnGf3 =
        gf3Reader.tryRead(gf3Path, gf3Bytes);
    expect(gf3OnGf3.matched, "GF3 reader matches binary SPE");
    expect(gf3OnGf3.data.channelCount == 256,
           "GF3 reader channel count is 256");
    expect(gf3OnGf3.data.headerBytes == 40,
           "GF3 reader header is 40 bytes");

    const ReaderResult asciiOnGf3 =
        asciiReader.tryRead(gf3Path, gf3Bytes);
    expect(!asciiOnGf3.matched,
           "ASCII reader rejects binary SPE");

    ReaderRegistry registry;
    registry.registerReader(
        std::unique_ptr<IReader>(new AsciiSPEReader()));
    registry.registerReader(
        std::unique_ptr<IReader>(new GF3Reader()));

    ReaderResult winner;
    std::string message;

    auto asciiResults =
        registry.runReaders(asciiPath, ".spe", asciiBytes);
    expect(registry.chooseWinner(asciiResults, winner, message),
           "Registry selects ASCII winner");
    expect(winner.readerName.find("ASCII") != std::string::npos,
           "ASCII winner name");

    auto gf3Results =
        registry.runReaders(gf3Path, ".spe", gf3Bytes);
    expect(registry.chooseWinner(gf3Results, winner, message),
           "Registry selects GF3 winner");
    expect(winner.readerName.find("GF3") != std::string::npos,
           "GF3 winner name");

    GenericDetector detector;
    SpectrumData forced;
    std::string forceReason;
    expect(
        detector.tryForceConvert(
            gf3Path, gf3Bytes, forced, forceReason),
        "Generic force-convert accepts GF3-like binary");
    expect(forced.channelCount == 256,
           "Forced conversion channel count is 256");

    if (failures == 0) {
        std::cout << "\nAll reader tests passed.\n";
        return 0;
    }

    std::cerr << "\n" << failures << " test(s) failed.\n";
    return 1;
}
