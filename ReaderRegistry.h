#ifndef UST_READER_REGISTRY_H
#define UST_READER_REGISTRY_H

#include "IReader.h"

#include <memory>
#include <string>
#include <vector>

namespace ust {

class ReaderRegistry {
public:
    void registerReader(std::unique_ptr<IReader> reader);

    std::vector<ReaderResult> runReaders(
        const std::string& path,
        const std::string& extension,
        const std::vector<unsigned char>& rawBytes) const;

    bool chooseWinner(
        const std::vector<ReaderResult>& results,
        ReaderResult& winner,
        std::string& message) const;

private:
    std::vector<std::unique_ptr<IReader>> readers_;
};

} // namespace ust

#endif
