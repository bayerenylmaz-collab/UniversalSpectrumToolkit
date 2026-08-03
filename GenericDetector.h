#ifndef UST_GENERIC_DETECTOR_H
#define UST_GENERIC_DETECTOR_H

#include "ByteUtils.h"
#include "SpectrumData.h"

#include <cstddef>
#include <string>
#include <vector>

namespace ust {

struct GenericCandidate {
    NumericType type = NumericType::Float32;
    ByteOrder order = ByteOrder::LittleEndian;
    std::size_t headerBytes = 0;
    std::size_t channels = 0;
    std::size_t bytesPerValue = 0;
    double finiteFraction = 0.0;
    double nonNegativeFraction = 0.0;
    double integerLikeFraction = 0.0;
    double zeroFraction = 0.0;
    double maximum = 0.0;
    double sum = 0.0;
    double structuralScore = 0.0;
};

class GenericDetector {
public:
    std::vector<GenericCandidate> scan(
        const std::vector<unsigned char>& rawBytes) const;

    void printCandidates(
        const std::vector<GenericCandidate>& candidates,
        std::size_t maxRows = 12) const;

    // Convert the best structurally plausible candidate.
    // Returns false if no candidate clears the score threshold.
    bool tryForceConvert(
        const std::string& path,
        const std::vector<unsigned char>& rawBytes,
        SpectrumData& out,
        std::string& reason,
        double minScore = 85.0) const;

private:
    GenericCandidate evaluate(
        const std::vector<unsigned char>& rawBytes,
        std::size_t headerBytes,
        std::size_t channels,
        NumericType type,
        ByteOrder order) const;

    SpectrumData materialize(
        const std::string& path,
        const std::vector<unsigned char>& rawBytes,
        const GenericCandidate& candidate) const;
};

} // namespace ust

#endif
