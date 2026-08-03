#ifndef UST_ORTEC_CHN_READER_H
#define UST_ORTEC_CHN_READER_H

#include "IReader.h"

namespace ust {

// Ortec / Maestro binary .chn integer spectrum
class OrtecCHNReader final : public IReader {
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
