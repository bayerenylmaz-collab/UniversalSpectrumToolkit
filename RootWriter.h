#ifndef UST_ROOT_WRITER_H
#define UST_ROOT_WRITER_H
#include "SpectrumData.h"
namespace ust { class RootWriter { public: static std::string defaultOutputPath(const std::string&); bool write(const SpectrumData&,const std::string&,bool drawSpectrum=true) const; }; }
#endif
