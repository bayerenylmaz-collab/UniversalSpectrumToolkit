#include "OrtecCHNReader.h"
#include "ByteUtils.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace ust {
namespace {

std::string asciiField(
    const std::vector<unsigned char>& raw,
    std::size_t offset,
    std::size_t length) {

    if (offset + length > raw.size()) {
        return "";
    }

    std::string out;
    out.reserve(length);

    for (std::size_t i = 0; i < length; ++i) {
        const unsigned char c = raw[offset + i];
        if (c == 0) {
            break;
        }
        if (c >= 32 && c <= 126) {
            out.push_back(static_cast<char>(c));
        }
    }

    return trim(out);
}

} // namespace

std::string OrtecCHNReader::name() const {
    return "Ortec binary CHN reader";
}

bool OrtecCHNReader::supportsExtension(
    const std::string& extension) const {

    return extension == ".chn";
}

ReaderResult OrtecCHNReader::tryRead(
    const std::string& path,
    const std::vector<unsigned char>& raw) const {

    ReaderResult result;
    result.readerName = name();

    constexpr std::size_t headerBytes = 32;
    constexpr std::size_t trailerMin = 2 + 2 + 12; // marker + pad + 3 floats

    if (raw.size() < headerBytes + 4) {
        result.reason = "File is too small for an Ortec CHN header.";
        return result;
    }

    if (mostlyText(raw)) {
        result.reason = "File looks like text; binary CHN layout skipped.";
        return result;
    }

    const std::int16_t fileType = static_cast<std::int16_t>(
        readU16(raw.data(), ByteOrder::LittleEndian));

    if (fileType != -1) {
        result.reason =
            "CHN signature mismatch: first int16 is not -1.";
        return result;
    }

    const std::int16_t channelOffset = static_cast<std::int16_t>(
        readU16(raw.data() + 28, ByteOrder::LittleEndian));

    const std::int16_t channelCountSigned =
        static_cast<std::int16_t>(
            readU16(raw.data() + 30, ByteOrder::LittleEndian));

    if (channelOffset < 0 || channelCountSigned <= 0) {
        result.reason = "Invalid channel offset or channel count in CHN header.";
        return result;
    }

    const std::size_t channelCount =
        static_cast<std::size_t>(channelCountSigned);

    if (!isCommonChannelCount(channelCount) &&
        channelCount < 16) {
        result.reason =
            "CHN channel count is implausibly small.";
        return result;
    }

    const std::size_t spectrumBytes = channelCount * 4;
    const std::size_t required = headerBytes + spectrumBytes;

    if (raw.size() < required) {
        result.reason =
            "File is truncated before the complete CHN spectrum payload.";
        return result;
    }

    const std::uint32_t realTimeTicks =
        readU32(raw.data() + 8, ByteOrder::LittleEndian);
    const std::uint32_t liveTimeTicks =
        readU32(raw.data() + 12, ByteOrder::LittleEndian);

    // Ortec stores timing in 20 ms ticks.
    const double realTime =
        static_cast<double>(realTimeTicks) * 0.02;
    const double liveTime =
        static_cast<double>(liveTimeTicks) * 0.02;

    const std::string datePart = asciiField(raw, 16, 8);
    const std::string timePart = asciiField(raw, 24, 4);
    const std::string secondsPart = asciiField(raw, 6, 2);

    std::string dateMeasured = datePart;
    if (!timePart.empty()) {
        if (!dateMeasured.empty()) {
            dateMeasured += " ";
        }
        if (timePart.size() >= 4) {
            dateMeasured += timePart.substr(0, 2);
            dateMeasured += ":";
            dateMeasured += timePart.substr(2, 2);
        } else {
            dateMeasured += timePart;
        }
        if (!secondsPart.empty()) {
            dateMeasured += ":";
            dateMeasured += secondsPart;
        }
    }

    std::vector<double> counts;
    counts.reserve(channelCount);

    std::size_t finite = 0;
    std::size_t nonNegative = 0;
    double maximum = 0.0;
    double sum = 0.0;

    for (std::size_t i = 0; i < channelCount; ++i) {
        const double value = decodeValue(
            raw.data() + headerBytes + 4 * i,
            NumericType::UInt32,
            ByteOrder::LittleEndian);

        counts.push_back(value);

        if (!std::isfinite(value)) {
            continue;
        }

        ++finite;
        if (value >= 0.0) {
            ++nonNegative;
        }

        maximum = std::max(maximum, value);
        sum += value;
    }

    const double n = static_cast<double>(channelCount);
    const double finiteFraction =
        static_cast<double>(finite) / n;
    const double nonNegativeFraction =
        static_cast<double>(nonNegative) / n;

    if (finiteFraction < 0.9999 ||
        nonNegativeFraction < 0.9999 ||
        maximum <= 0.0 ||
        sum <= 0.0) {
        result.reason =
            "CHN spectrum failed basic count sanity checks.";
        return result;
    }

    double energyA = 0.0;
    double energyB = 0.0;
    double energyC = 0.0;
    bool hasEnergy = false;

    const std::size_t trailerOffset = required;
    if (raw.size() >= trailerOffset + trailerMin) {
        const std::int16_t marker = static_cast<std::int16_t>(
            readU16(
                raw.data() + trailerOffset,
                ByteOrder::LittleEndian));

        if (marker == -102) {
            energyA = decodeValue(
                raw.data() + trailerOffset + 4,
                NumericType::Float32,
                ByteOrder::LittleEndian);
            energyB = decodeValue(
                raw.data() + trailerOffset + 8,
                NumericType::Float32,
                ByteOrder::LittleEndian);
            energyC = decodeValue(
                raw.data() + trailerOffset + 12,
                NumericType::Float32,
                ByteOrder::LittleEndian);

            if (std::isfinite(energyA) &&
                std::isfinite(energyB) &&
                std::isfinite(energyC) &&
                std::abs(energyB) > 0.0) {
                hasEnergy = true;
            }
        }
    }

    result.matched = true;
    result.score = 95.0;
    result.reason =
        "Matched Ortec/Maestro binary CHN signature "
        "(-1) with integer channel payload.";

    SpectrumData& data = result.data;
    data.counts = std::move(counts);
    data.rawBytes = raw;
    data.sourceFile = path;
    data.extension = extensionOf(path);
    data.detectedFormat = "Ortec binary CHN";
    data.readerName = name();
    data.numericType = "uint32";
    data.byteOrder = "little-endian";
    data.decisionReason = result.reason;
    data.dateMeasured = dateMeasured;
    data.fileSize = raw.size();
    data.headerBytes = headerBytes;
    data.channelCount = channelCount;
    data.firstChannel = static_cast<std::size_t>(channelOffset);
    data.lastChannel =
        static_cast<std::size_t>(channelOffset) + channelCount - 1;
    data.bytesPerValue = 4;
    data.liveTime = liveTime;
    data.realTime = realTime;
    data.energyA = energyA;
    data.energyB = energyB;
    data.energyC = energyC;
    data.hasEnergyCalibration = hasEnergy;
    data.readerScore = result.score;
    data.valid = true;

    return result;
}

} // namespace ust
