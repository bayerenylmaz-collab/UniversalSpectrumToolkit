#ifndef UST_GF3_READER_H
#define UST_GF3_READER_H
#include "IReader.h"
namespace ust { class GF3Reader final: public IReader { public: std::string name() const override; bool supportsExtension(const std::string&) const override; ReaderResult tryRead(const std::string&,const std::vector<unsigned char>&) const override; }; }
#endif
