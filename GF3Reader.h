#ifndef UST_GF3_READER_H
#define UST_GF3_READER_H

#include "IReader.h"

namespace ust {

// Validated GF3-like binary SPE:
// fixed 40-byte header + float32 little-endian MCA channels
class GF3Reader final : public IReader {
public:
    std::string name() const override;

    bool supportsExtension(
        const std::string& extension) const override;

    ReaderResult tryRead(
        const std::string& path,
        const std::vector<unsigned char>& rawBytes) const override;
};

} // namespace ust

#endif
