#ifndef UST_BYTE_UTILS_H
#define UST_BYTE_UTILS_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace ust {

enum class NumericType {
    UInt16, Int16, UInt32, Int32, Float32, Float64
};

enum class ByteOrder {
    LittleEndian, BigEndian
};

inline std::string trim(const std::string& s) {
    const auto first = std::find_if_not(
        s.begin(), s.end(),
        [](unsigned char c) { return std::isspace(c); });

    if (first == s.end()) return "";

    const auto last = std::find_if_not(
        s.rbegin(), s.rend(),
        [](unsigned char c) { return std::isspace(c); }).base();

    return std::string(first, last);
}

inline std::string lower(std::string s) {
    std::transform(
        s.begin(), s.end(), s.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    return s;
}

inline std::string extensionOf(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    const std::size_t dot = path.find_last_of('.');

    if (dot == std::string::npos ||
        (slash != std::string::npos && dot < slash)) {
        return "";
    }

    return lower(path.substr(dot));
}

inline std::vector<unsigned char> readAllBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);

    if (!in) {
        throw std::runtime_error("Cannot open input file: " + path);
    }

    in.seekg(0, std::ios::end);
    const std::streamoff end = in.tellg();

    if (end <= 0) {
        throw std::runtime_error("Input file is empty or unreadable.");
    }

    in.seekg(0, std::ios::beg);

    std::vector<unsigned char> bytes(
        static_cast<std::size_t>(end));

    in.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(end));

    if (!in) {
        throw std::runtime_error(
            "Could not read the complete input file.");
    }

    return bytes;
}

inline bool mostlyText(const std::vector<unsigned char>& bytes) {
    const std::size_t n =
        std::min<std::size_t>(bytes.size(), 8192);

    if (n == 0) return false;

    std::size_t printable = 0;

    for (std::size_t i = 0; i < n; ++i) {
        const unsigned char c = bytes[i];

        if (c == '\n' || c == '\r' || c == '\t' ||
            (c >= 32 && c <= 126)) {
            ++printable;
        }
    }

    return static_cast<double>(printable) /
           static_cast<double>(n) > 0.92;
}

inline std::uint16_t readU16(
    const unsigned char* p,
    ByteOrder order) {

    if (order == ByteOrder::LittleEndian) {
        return static_cast<std::uint16_t>(p[0]) |
               (static_cast<std::uint16_t>(p[1]) << 8);
    }

    return (static_cast<std::uint16_t>(p[0]) << 8) |
           static_cast<std::uint16_t>(p[1]);
}

inline std::uint32_t readU32(
    const unsigned char* p,
    ByteOrder order) {

    if (order == ByteOrder::LittleEndian) {
        return static_cast<std::uint32_t>(p[0]) |
               (static_cast<std::uint32_t>(p[1]) << 8) |
               (static_cast<std::uint32_t>(p[2]) << 16) |
               (static_cast<std::uint32_t>(p[3]) << 24);
    }

    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}

inline std::uint64_t readU64(
    const unsigned char* p,
    ByteOrder order) {

    std::uint64_t value = 0;

    if (order == ByteOrder::LittleEndian) {
        for (int i = 7; i >= 0; --i) {
            value = (value << 8) |
                    static_cast<std::uint64_t>(p[i]);
        }
    } else {
        for (int i = 0; i < 8; ++i) {
            value = (value << 8) |
                    static_cast<std::uint64_t>(p[i]);
        }
    }

    return value;
}

inline std::size_t bytesPerValue(NumericType type) {
    switch (type) {
        case NumericType::UInt16:
        case NumericType::Int16:
            return 2;

        case NumericType::UInt32:
        case NumericType::Int32:
        case NumericType::Float32:
            return 4;

        case NumericType::Float64:
            return 8;
    }

    return 0;
}

inline const char* numericTypeName(NumericType type) {
    switch (type) {
        case NumericType::UInt16:  return "uint16";
        case NumericType::Int16:   return "int16";
        case NumericType::UInt32:  return "uint32";
        case NumericType::Int32:   return "int32";
        case NumericType::Float32: return "float32";
        case NumericType::Float64: return "float64";
    }

    return "unknown";
}

inline const char* byteOrderName(ByteOrder order) {
    return order == ByteOrder::LittleEndian
        ? "little-endian"
        : "big-endian";
}

inline double decodeValue(
    const unsigned char* p,
    NumericType type,
    ByteOrder order) {

    switch (type) {
        case NumericType::UInt16:
            return static_cast<double>(readU16(p, order));

        case NumericType::Int16:
            return static_cast<double>(
                static_cast<std::int16_t>(readU16(p, order)));

        case NumericType::UInt32:
            return static_cast<double>(readU32(p, order));

        case NumericType::Int32:
            return static_cast<double>(
                static_cast<std::int32_t>(readU32(p, order)));

        case NumericType::Float32: {
            const std::uint32_t bits = readU32(p, order);
            float value = 0.0f;
            std::memcpy(&value, &bits, sizeof(value));
            return static_cast<double>(value);
        }

        case NumericType::Float64: {
            const std::uint64_t bits = readU64(p, order);
            double value = 0.0;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }
    }

    return std::numeric_limits<double>::quiet_NaN();
}

inline bool isCommonChannelCount(std::size_t n) {
    static const std::vector<std::size_t> common = {
        256, 512, 1024, 2048, 4096,
        8192, 16384, 32768, 65536
    };

    return std::find(common.begin(), common.end(), n)
        != common.end();
}

inline bool isIntegerLike(double x) {
    if (!std::isfinite(x)) return false;

    const double scale = std::max(1.0, std::abs(x));

    return std::abs(x - std::round(x))
        <= 1e-5 * scale;
}

} // namespace ust

#endif
