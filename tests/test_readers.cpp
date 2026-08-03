// Standalone reader tests (no ROOT required).
// Build from this directory:
//   g++ -std=c++17 -O2 -o test_readers test_readers.cpp
//   ./test_readers

#include "../SpectrumData.h"
#include "../ByteUtils.h"
#include "../AsciiSPEReader.h"
#include "../GF3Reader.h"
#include "../OrtecCHNReader.h"
#include "../ReaderRegistry.h"
#include "../GenericDetector.h"

#include "../AsciiSPEReader.cxx"
#include "../GF3Reader.cxx"
#include "../OrtecCHNReader.cxx"
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
    const std::string chnPath = "../samples/ortec_demo.chn";

    const auto asciiBytes = readAllBytes(asciiPath);
    const auto gf3Bytes = readAllBytes(gf3Path);
    const auto chnBytes = readAllBytes(chnPath);

    AsciiSPEReader asciiReader;
    GF3Reader gf3Reader;
    OrtecCHNReader chnReader;

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

    const ReaderResult chnOnChn =
        chnReader.tryRead(chnPath, chnBytes);
    expect(chnOnChn.matched, "CHN reader matches Ortec CHN");
    expect(chnOnChn.data.channelCount == 256,
           "CHN reader channel count is 256");
    expect(chnOnChn.data.headerBytes == 32,
           "CHN reader header is 32 bytes");
    expect(std::abs(chnOnChn.data.liveTime - 100.0) < 1e-9,
           "CHN reader live time is 100 s");
    expect(std::abs(chnOnChn.data.realTime - 101.5) < 1e-9,
           "CHN reader real time is 101.5 s");
    expect(chnOnChn.data.hasEnergyCalibration,
           "CHN reader energy calibration parsed");
    expect(std::abs(chnOnChn.data.energyB - 1.25) < 1e-6,
           "CHN reader energy slope is 1.25");
    expect(chnOnChn.data.counts[51] == 1800.0,
           "CHN reader peak channel value");

    const ReaderResult chnOnSpe =
        chnReader.tryRead(gf3Path, gf3Bytes);
    expect(!chnOnSpe.matched,
           "CHN reader rejects GF3 SPE by extension path logic elsewhere");

    // Direct tryRead ignores extension; signature should reject GF3 payload.
    // GF3 starts with ASCII-ish 'GF3' bytes, not int16 -1, so no match.
    expect(!chnOnSpe.matched, "CHN reader rejects non-CHN binary");

    ReaderRegistry registry;
    registry.registerReader(
        std::unique_ptr<IReader>(new AsciiSPEReader()));
    registry.registerReader(
        std::unique_ptr<IReader>(new GF3Reader()));
    registry.registerReader(
        std::unique_ptr<IReader>(new OrtecCHNReader()));

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

    auto chnResults =
        registry.runReaders(chnPath, ".chn", chnBytes);
    expect(registry.chooseWinner(chnResults, winner, message),
           "Registry selects CHN winner");
    expect(winner.readerName.find("CHN") != std::string::npos,
           "CHN winner name");

    // SPE readers must not claim .chn files.
    auto speReadersOnChn =
        registry.runReaders(chnPath, ".chn", chnBytes);
    expect(speReadersOnChn.size() == 1,
           "Only CHN reader runs for .chn extension");

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
