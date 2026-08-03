#ifndef UST_SPECTRUM_DATA_H
#define UST_SPECTRUM_DATA_H

#include <cstddef>
#include <string>
#include <vector>

namespace ust {

struct SpectrumData {
    std::vector<double> counts;
    std::vector<unsigned char> rawBytes;

    std::string sourceFile;
    std::string extension;
    std::string detectedFormat;
    std::string readerName;
    std::string numericType;
    std::string byteOrder;
    std::string decisionReason;

    // Optional Ortec / MCA metadata
    std::string specId;
    std::string dateMeasured;

    std::size_t fileSize = 0;
    std::size_t headerBytes = 0;
    std::size_t channelCount = 0;
    std::size_t bytesPerValue = 0;
    std::size_t firstChannel = 0;
    std::size_t lastChannel = 0;

    double readerScore = 0.0;
    double liveTime = 0.0;
    double realTime = 0.0;

    // Energy calibration: E = a + b*ch + c*ch^2
    double energyA = 0.0;
    double energyB = 0.0;
    double energyC = 0.0;
    bool hasEnergyCalibration = false;

    bool valid = false;
};

struct ReaderResult {
    SpectrumData data;
    bool matched = false;
    double score = 0.0;
    std::string readerName;
    std::string reason;
};

} // namespace ust

#endif
