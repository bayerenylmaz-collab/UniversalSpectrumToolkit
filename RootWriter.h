#ifndef UST_ROOT_WRITER_H
#define UST_ROOT_WRITER_H

#include "SpectrumData.h"

#include <string>

namespace ust {

class RootWriter {
public:
    static std::string defaultOutputPath(const std::string& inputPath);

    bool write(
        const SpectrumData& data,
        const std::string& outputPath,
        bool drawSpectrum = true,
        bool logY = true) const;
};

} // namespace ust

#endif
