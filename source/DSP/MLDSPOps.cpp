

// MLDSPOps.cpp
// MLTEST

#include "MLDSPOps.h"

#ifdef MANUAL_ALIGN_DSPVECTOR

float* ml::DSPVectorAlignFloatPointer(const float* p)
{
	uintptr_t pM = (uintptr_t)p;
	pM += (uintptr_t)(ml::kDSPVectorAlignBytes - 1);
	pM &= ml::kDSPVectorBytesAlignMask;
	return reinterpret_cast<float*>(pM);
}


int* ml::DSPVectorAlignIntPointer(const int* p)
{
	uintptr_t pM = (uintptr_t)p;
	pM += (uintptr_t)(ml::kDSPVectorAlignBytes - 1);
	pM &= ml::kDSPVectorBytesAlignMask;
	return reinterpret_cast<int*>(pM);
}

#else


#endif

 