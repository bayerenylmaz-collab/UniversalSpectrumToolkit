#include "GF3Reader.h"
#include "ByteUtils.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ust {

std::string GF3Reader::name() const {
    return "GF3-like binary SPE reader";
}

bool GF3Reader::supportsExtension(
    const std::string& extension) const {

    return extension == ".spe";
}

ReaderResult GF3Reader::tryRead(
    const std::string& path,
    const std::vector<unsigned char>& raw) const {

    ReaderResult result;
    result.readerName = name();

    // Text Ortec SPE files must not be claimed by this reader.
    if (mostlyText(raw)) {
        result.reason =
            "File looks like text; binary GF3 layout skipped.";
        return result;
    }

    constexpr std::size_t header = 40;
    constexpr std::size_t bytesPerValue = 4;

    if (raw.size() <= header) {
        result.reason =
            "File is too small for the validated GF3-like layout.";
        return result;
    }

    const std::size_t payload = raw.size() - header;
    if (payload % bytesPerValue != 0) {
        result.reason =
            "Payload after the 40-byte header is not divisible by 4.";
        return result;
    }

    const std::size_t channels = payload / bytesPerValue;
    if (!isCommonChannelCount(channels)) {
        result.reason =
            "The inferred channel count is not a supported "
            "common MCA channel count.";
        return result;
    }

    std::vector<double> counts;
    counts.reserve(channels);

    std::size_t finite = 0;
    std::size_t nonNegative = 0;
    std::size_t integerLike = 0;
    double maximum = 0.0;
    double sum = 0.0;

    for (std::size_t i = 0; i < channels; ++i) {
        const double value = decodeValue(
            raw.data() + header + bytesPerValue * i,
            NumericType::Float32,
            ByteOrder::LittleEndian);

        counts.push_back(value);

        if (!std::isfinite(value)) {
            continue;
        }

        ++finite;
        if (value >= 0.0) {
            ++nonNegative;
        }
        if (isIntegerLike(value)) {
            ++integerLike;
        }

        maximum = std::max(maximum, value);
        sum += value;
    }

    const double n = static_cast<double>(channels);
    const double finiteFraction =
        static_cast<double>(finite) / n;
    const double nonNegativeFraction =
        static_cast<double>(nonNegative) / n;
    const double integerLikeFraction =
        static_cast<double>(integerLike) / n;

    const bool ok =
        finiteFraction > 0.9999 &&
        nonNegativeFraction > 0.9999 &&
        integerLikeFraction > 0.99 &&
        maximum > 0.0 &&
        maximum < 1e12 &&
        sum > 0.0 &&
        std::isfinite(sum);

    if (!ok) {
        std::ostringstream message;
        message
            << "The 40-byte/float32-LE layout failed structural "
            << "checks (finite=" << finiteFraction
            << ", non-negative=" << nonNegativeFraction
            << ", integer-like=" << integerLikeFraction << ").";
        result.reason = message.str();
        return result;
    }

    // Slightly below tagged ASCII so text SPE always wins cleanly.
    result.matched = true;
    result.score = 90.0;
    result.reason =
        "Matched the validated 40-byte header plus common-channel "
        "float32 little-endian layout.";

    SpectrumData& data = result.data;
    data.counts = std::move(counts);
    data.rawBytes = raw;
    data.sourceFile = path;
    data.extension = extensionOf(path);
    data.detectedFormat = "GF3-like binary SPE";
    data.readerName = name();
    data.numericType = "float32";
    data.byteOrder = "little-endian";
    data.decisionReason = result.reason;
    data.fileSize = raw.size();
    data.headerBytes = header;
    data.channelCount = channels;
    data.firstChannel = 0;
    data.lastChannel = channels > 0 ? channels - 1 : 0;
    data.bytesPerValue = bytesPerValue;
    data.readerScore = result.score;
    data.valid = true;

    return result;
}

} // namespace ust
