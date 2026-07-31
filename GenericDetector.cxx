#include "GenericDetector.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace ust {

GenericCandidate GenericDetector::evaluate(
    const std::vector<unsigned char>& rawBytes,
    std::size_t headerBytes,
    std::size_t channels,
    NumericType type,
    ByteOrder order) const {

    GenericCandidate candidate;

    candidate.type = type;
    candidate.order = order;
    candidate.headerBytes = headerBytes;
    candidate.channels = channels;
    candidate.bytesPerValue = bytesPerValue(type);

    if (candidate.bytesPerValue == 0 ||
        headerBytes > rawBytes.size() ||
        channels >
            (rawBytes.size() - headerBytes) /
            candidate.bytesPerValue) {

        return candidate;
    }

    std::size_t finite = 0;
    std::size_t nonNegative = 0;
    std::size_t integerLike = 0;
    std::size_t zeros = 0;

    double maximum = 0.0;
    double sum = 0.0;

    for (std::size_t i = 0; i < channels; ++i) {
        const double value = decodeValue(
            rawBytes.data() +
                headerBytes +
                candidate.bytesPerValue * i,
            type,
            order);

        if (!std::isfinite(value)) continue;

        ++finite;

        if (value >= 0.0) ++nonNegative;
        if (value == 0.0) ++zeros;
        if (isIntegerLike(value)) ++integerLike;

        maximum = std::max(maximum, value);
        sum += value;
    }

    const double n = static_cast<double>(channels);

    candidate.finiteFraction =
        static_cast<double>(finite) / n;

    candidate.nonNegativeFraction =
        static_cast<double>(nonNegative) / n;

    candidate.integerLikeFraction =
        static_cast<double>(integerLike) / n;

    candidate.zeroFraction =
        static_cast<double>(zeros) / n;

    candidate.maximum = maximum;
    candidate.sum = sum;

    double score = 0.0;

    score += 35.0 * candidate.finiteFraction;
    score += 30.0 * candidate.nonNegativeFraction;
    score += 15.0 * candidate.integerLikeFraction;

    if (maximum > 0.0 && maximum < 1e12) {
        score += 10.0;
    }

    if (sum > 0.0 &&
        std::isfinite(sum) &&
        sum < 1e18) {
        score += 5.0;
    }

    if (candidate.zeroFraction > 0.0 &&
        candidate.zeroFraction < 0.999) {
        score += 3.0;
    }

    if (isCommonChannelCount(channels)) {
        score += 2.0;
    }

    candidate.structuralScore =
        std::min(100.0, score);

    return candidate;
}

std::vector<GenericCandidate> GenericDetector::scan(
    const std::vector<unsigned char>& rawBytes) const {

    static const std::vector<NumericType> types = {
        NumericType::UInt16,
        NumericType::Int16,
        NumericType::UInt32,
        NumericType::Int32,
        NumericType::Float32,
        NumericType::Float64
    };

    static const std::vector<ByteOrder> orders = {
        ByteOrder::LittleEndian,
        ByteOrder::BigEndian
    };

    static const std::vector<std::size_t> channels = {
        256, 512, 1024, 2048, 4096,
        8192, 16384, 32768, 65536
    };

    std::vector<GenericCandidate> candidates;

    for (NumericType type : types) {
        const std::size_t valueBytes =
            bytesPerValue(type);

        for (std::size_t channelCount : channels) {
            const std::size_t payload =
                channelCount * valueBytes;

            if (payload > rawBytes.size()) {
                continue;
            }

            const std::size_t header =
                rawBytes.size() - payload;

            if (header > 65536) {
                continue;
            }

            for (ByteOrder order : orders) {
                candidates.push_back(
                    evaluate(
                        rawBytes,
                        header,
                        channelCount,
                        type,
                        order));
            }
        }
    }

    std::sort(
        candidates.begin(), candidates.end(),
        [](const GenericCandidate& a,
           const GenericCandidate& b) {

            if (a.structuralScore != b.structuralScore) {
                return a.structuralScore >
                       b.structuralScore;
            }

            if (a.headerBytes != b.headerBytes) {
                return a.headerBytes <
                       b.headerBytes;
            }

            return a.channels < b.channels;
        });

    return candidates;
}

void GenericDetector::printCandidates(
    const std::vector<GenericCandidate>& candidates,
    std::size_t maxRows) const {

    std::cout
        << "\nNo registered reader recognized "
        << "this extension/structure.\n"
        << "The generic detector found structurally "
        << "readable candidates.\n"
        << "These are NOT automatic format decisions.\n\n";

    std::cout
        << std::left
        << std::setw(6)  << "Rank"
        << std::setw(10) << "Header"
        << std::setw(11) << "Channels"
        << std::setw(11) << "Type"
        << std::setw(16) << "Byte order"
        << std::setw(10) << "Score"
        << '\n';

    std::cout << std::string(64, '-') << '\n';

    const std::size_t rows =
        std::min(maxRows, candidates.size());

    for (std::size_t i = 0; i < rows; ++i) {
        const GenericCandidate& candidate =
            candidates[i];

        std::cout
            << std::left
            << std::setw(6)  << (i + 1)
            << std::setw(10) << candidate.headerBytes
            << std::setw(11) << candidate.channels
            << std::setw(11)
            << numericTypeName(candidate.type)
            << std::setw(16)
            << byteOrderName(candidate.order)
            << std::fixed
            << std::setprecision(2)
            << std::setw(10)
            << candidate.structuralScore
            << '\n';
    }

    std::cout
        << "\nConversion stopped safely because "
        << "the format is unknown or ambiguous.\n"
        << "A new registered reader can be added later, "
        << "or a future explicit-layout interface can be used.\n";
}

} // namespace ust
