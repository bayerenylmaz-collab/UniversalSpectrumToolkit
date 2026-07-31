#ifndef UST_IREADER_H
#define UST_IREADER_H
#include "SpectrumData.h"
#include <string>
#include <vector>
namespace ust {
class IReader {
public:
    virtual ~IReader() = default;
    virtual std::string name() const = 0;
    virtual bool supportsExtension(const std::string& extension) const = 0;
    virtual ReaderResult tryRead(const std::string& path, const std::vector<unsigned char>& rawBytes) const = 0;
};
}
#endif
