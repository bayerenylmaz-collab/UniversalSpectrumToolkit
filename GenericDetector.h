#ifndef UST_GENERIC_DETECTOR_H
#define UST_GENERIC_DETECTOR_H
#include "ByteUtils.h"
namespace ust { struct GenericCandidate { NumericType type=NumericType::Float32; ByteOrder order=ByteOrder::LittleEndian; std::size_t headerBytes=0,channels=0,bytesPerValue=0; double finiteFraction=0,nonNegativeFraction=0,integerLikeFraction=0,zeroFraction=0,maximum=0,sum=0,structuralScore=0; }; class GenericDetector { GenericCandidate evaluate(const std::vector<unsigned char>&,std::size_t,std::size_t,NumericType,ByteOrder) const; public: std::vector<GenericCandidate> scan(const std::vector<unsigned char>&) const; void printCandidates(const std::vector<GenericCandidate>&,std::size_t maxRows=12) const; }; }
#endif
