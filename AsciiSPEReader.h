#ifndef UST_ASCII_SPE_READER_H
#define UST_ASCII_SPE_READER_H
#include "IReader.h"
namespace ust { class AsciiSPEReader final: public IReader { public: std::string name() const override; bool supportsExtension(const std::string&) const override; ReaderResult tryRead(const std::string&,const std::vector<unsigned char>&) const override; }; }
#endif
