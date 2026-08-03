#ifndef UST_ASCII_SPE_READER_H
#define UST_ASCII_SPE_READER_H

#include "IReader.h"

namespace ust {

// Ortec / Maestro style tagged ASCII SPE ($DATA:, $MEAS_TIM:, ...)
class AsciiSPEReader final : public IReader {
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
