#include "AsciiSPEReader.h"
#include "ByteUtils.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace ust {
namespace {

std::vector<std::string> splitLines(
    const std::vector<unsigned char>& raw) {

    std::string text(
        reinterpret_cast<const char*>(raw.data()),
        raw.size());

    std::vector<std::string> lines;
    std::string current;

    for (char ch : text) {
        if (ch == '\n') {
            if (!current.empty() && current.back() == '\r') {
                current.pop_back();
            }
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    if (!current.empty()) {
        if (current.back() == '\r') {
            current.pop_back();
        }
        lines.push_back(current);
    }

    return lines;
}

std::string sectionName(const std::string& line) {
    const std::string t = lower(trim(line));
    if (t.empty() || t[0] != '$') {
        return "";
    }

    std::string name = t.substr(1);
    if (!name.empty() && name.back() == ':') {
        name.pop_back();
    }
    return name;
}

bool parseChannelRange(
    const std::string& line,
    long& first,
    long& last) {

    std::istringstream ss(line);
    if (!(ss >> first >> last)) {
        return false;
    }
    return first >= 0 && last >= first;
}

bool readNumericLine(
    const std::string& line,
    std::vector<double>& counts,
    std::size_t expected) {

    const std::string t = trim(line);
    if (t.empty() || t[0] == '$') {
        return false;
    }

    std::istringstream ss(t);
    double value = 0.0;
    bool gotAny = false;

    while (ss >> value) {
        counts.push_back(value);
        gotAny = true;
        if (counts.size() == expected) {
            break;
        }
    }

    return gotAny;
}

} // namespace

std::string AsciiSPEReader::name() const {
    return "Tagged ASCII SPE reader";
}

bool AsciiSPEReader::supportsExtension(
    const std::string& extension) const {

    return extension == ".spe";
}

ReaderResult AsciiSPEReader::tryRead(
    const std::string& path,
    const std::vector<unsigned char>& raw) const {

    ReaderResult result;
    result.readerName = name();

    if (!mostlyText(raw)) {
        result.reason = "File is not predominantly text.";
        return result;
    }

    const std::vector<std::string> lines = splitLines(raw);

    std::size_t dataTag = lines.size();
    std::string specId;
    std::string dateMeasured;
    double liveTime = 0.0;
    double realTime = 0.0;
    double energyA = 0.0;
    double energyB = 0.0;
    double energyC = 0.0;
    bool hasEnergy = false;

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string section = sectionName(lines[i]);
        if (section.empty()) {
            continue;
        }

        if (section == "data") {
            dataTag = i;
            continue;
        }

        if (section == "spec_id" && i + 1 < lines.size()) {
            specId = trim(lines[i + 1]);
            continue;
        }

        if (section == "date_mea" && i + 1 < lines.size()) {
            dateMeasured = trim(lines[i + 1]);
            continue;
        }

        if (section == "meas_tim" && i + 1 < lines.size()) {
            std::istringstream ss(lines[i + 1]);
            ss >> liveTime >> realTime;
            continue;
        }

        // Ortec: $ENER_FIT: then one line with a b  (and optional c)
        if (section == "ener_fit" && i + 1 < lines.size()) {
            std::istringstream ss(lines[i + 1]);
            if (ss >> energyA >> energyB) {
                ss >> energyC;
                hasEnergy = true;
            }
            continue;
        }

        // Ortec: $MCA_CAL: then degree, then coefficients
        if (section == "mca_cal" && i + 2 < lines.size()) {
            int degree = 0;
            std::istringstream deg(lines[i + 1]);
            if (!(deg >> degree) || degree < 1) {
                continue;
            }

            std::istringstream coeff(lines[i + 2]);
            double a = 0.0;
            double b = 0.0;
            double c = 0.0;

            if (coeff >> a >> b) {
                coeff >> c;
                energyA = a;
                energyB = b;
                energyC = c;
                hasEnergy = true;
            }
        }
    }

    if (dataTag == lines.size()) {
        result.reason = "No $DATA section was found.";
        return result;
    }

    std::size_t rangeLine = dataTag + 1;
    while (rangeLine < lines.size() &&
           trim(lines[rangeLine]).empty()) {
        ++rangeLine;
    }

    if (rangeLine >= lines.size()) {
        result.reason = "$DATA section has no channel-range line.";
        return result;
    }

    long first = 0;
    long last = -1;
    if (!parseChannelRange(lines[rangeLine], first, last)) {
        result.reason = "Invalid channel range after $DATA.";
        return result;
    }

    const std::size_t expected =
        static_cast<std::size_t>(last - first + 1);

    std::vector<double> counts;
    counts.reserve(expected);

    for (std::size_t i = rangeLine + 1;
         i < lines.size() && counts.size() < expected;
         ++i) {

        const std::string t = trim(lines[i]);
        if (t.empty()) {
            continue;
        }
        if (t[0] == '$') {
            break;
        }
        if (!readNumericLine(lines[i], counts, expected)) {
            break;
        }
    }

    if (counts.size() != expected) {
        std::ostringstream message;
        message
            << "$DATA declares " << expected
            << " channels, but " << counts.size()
            << " values were read.";
        result.reason = message.str();
        return result;
    }

    const bool allValid = std::all_of(
        counts.begin(),
        counts.end(),
        [](double x) {
            return std::isfinite(x) && x >= 0.0;
        });

    if (!allValid) {
        result.reason =
            "$DATA contains invalid or negative counts.";
        return result;
    }

    result.matched = true;
    result.score = 100.0;
    result.reason =
        "Recognized Ortec/Maestro-style tagged ASCII SPE "
        "with a complete $DATA channel range.";

    SpectrumData& data = result.data;
    data.counts = std::move(counts);
    data.rawBytes = raw;
    data.sourceFile = path;
    data.extension = extensionOf(path);
    data.detectedFormat = "Tagged ASCII SPE (Ortec/Maestro)";
    data.readerName = name();
    data.numericType = "ASCII numeric text";
    data.byteOrder = "not applicable";
    data.decisionReason = result.reason;
    data.specId = specId;
    data.dateMeasured = dateMeasured;
    data.fileSize = raw.size();
    data.channelCount = expected;
    data.firstChannel = static_cast<std::size_t>(first);
    data.lastChannel = static_cast<std::size_t>(last);
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
