#ifndef UST_SPECTRUM_DATA_H
#define UST_SPECTRUM_DATA_H
#include <cstddef>
#include <string>
#include <vector>
namespace ust {
struct SpectrumData {
    std::vector<double> counts;
    std::vector<unsigned char> rawBytes;
    std::string sourceFile, extension, detectedFormat, readerName;
    std::string numericType, byteOrder, decisionReason;
    std::size_t fileSize=0, headerBytes=0, channelCount=0, bytesPerValue=0;
    double readerScore=0.0;
    bool valid=false;
};
struct ReaderResult {
    SpectrumData data;
    bool matched=false;
    double score=0.0;
    std::string readerName, reason;
};
}
#endif
