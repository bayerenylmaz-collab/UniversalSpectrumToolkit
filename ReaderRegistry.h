#ifndef UST_READER_REGISTRY_H
#define UST_READER_REGISTRY_H
#include "IReader.h"
#include <memory>
namespace ust { class ReaderRegistry { std::vector<std::unique_ptr<IReader>> readers_; public: void registerReader(std::unique_ptr<IReader>); std::vector<ReaderResult> runReaders(const std::string&,const std::string&,const std::vector<unsigned char>&) const; bool chooseWinner(const std::vector<ReaderResult>&,ReaderResult&,std::string&) const; }; }
#endif
