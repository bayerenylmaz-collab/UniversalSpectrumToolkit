#include "ReaderRegistry.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ust {

void ReaderRegistry::registerReader(
    std::unique_ptr<IReader> reader) {

    if (reader) {
        readers_.push_back(std::move(reader));
    }
}

std::vector<ReaderResult> ReaderRegistry::runReaders(
    const std::string& path,
    const std::string& extension,
    const std::vector<unsigned char>& rawBytes) const {

    std::vector<ReaderResult> out;

    for (const auto& reader : readers_) {
        if (reader->supportsExtension(extension)) {
            out.push_back(reader->tryRead(path, rawBytes));
        }
    }

    return out;
}

bool ReaderRegistry::chooseWinner(
    const std::vector<ReaderResult>& results,
    ReaderResult& winner,
    std::string& message) const {

    std::vector<ReaderResult> matched;

    for (const ReaderResult& result : results) {
        if (result.matched && result.data.valid) {
            matched.push_back(result);
        }
    }

    if (matched.empty()) {
        message = "No known reader matched.";
        return false;
    }

    std::sort(
        matched.begin(),
        matched.end(),
        [](const ReaderResult& a, const ReaderResult& b) {
            if (std::abs(a.score - b.score) > 1e-9) {
                return a.score > b.score;
            }
            // Prefer more specific / tagged formats on ties.
            return a.readerName < b.readerName;
        });

    if (matched.size() > 1 &&
        std::abs(matched[0].score - matched[1].score) < 1e-9) {

        std::ostringstream stream;
        stream
            << "Ambiguous known-reader result: "
            << matched[0].readerName << " and "
            << matched[1].readerName
            << " received equal scores.";
        message = stream.str();
        return false;
    }

    winner = matched.front();
    message = "Known reader selected: " + winner.readerName;
    return true;
}

} // namespace ust
