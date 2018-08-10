

// MLDSPOps.cpp
// MLTEST

#include "MLDSPOps.h"

#ifdef MANUAL_ALIGN_DSPVECTOR

float* ml::DSPVectorAlignFloatPointer(const float* p)
{
	std::cout << "ats input: " << std::hex << (unsigned long)(p) << "\n";

	uintptr_t pM = (uintptr_t)p;
	pM += (uintptr_t)(ml::kDSPVectorAlignBytes - 1);
	pM &= ml::kDSPVectorAlignMask;

	std::cout << "    output: " << std::hex << (unsigned long)(pM) << std::dec << "\n";

	return reinterpret_cast<float*>(pM);
}


int* ml::DSPVectorAlignIntPointer(const int* p)
{
	std::cout << "ats input: " << std::hex << (unsigned long)(p) << "\n";

	uintptr_t pM = (uintptr_t)p;
	pM += (uintptr_t)(ml::kDSPVectorAlignBytes - 1);
	pM &= ml::kDSPVectorAlignMask;

	std::cout << "    output: " << std::hex << (unsigned long)(pM) << std::dec << "\n";

	return reinterpret_cast<int*>(pM);
}

#else


#endif

