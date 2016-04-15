//
//  MLDSPMath.h
//  madronalib
//
//  Created by Randy Jones on 4/14/2016
//
//

#pragma once

// Here is the global process chunk size, an important constant.
constexpr int kProcessChunkSize = 64;

// Load definitions for low-level SIMD math. 
// These must define vecFloat, vecInt, and a bunch of operations on them.
// We are currently only using 4-element vectors on both SSE and NEON.
#if(defined  __ARM_NEON) || (defined __ARM_NEON__)

// NEON 

// NEON headers?

constexpr int kDSPVectorSizeFloat = kProcessChunkSize;
constexpr uintptr_t kfloatsPerSIMDVectorBits = 2;
constexpr uintptr_t kSIMDVecSize = 1 << kfloatsPerSIMDVectorBits;
constexpr int kDSPVectorSizeSIMD = kDSPVectorSizeFloat / kSIMDVecSize;

#include "MLDSPMathNEON.h"

#else 

// SSE2

#include <xmmintrin.h>

constexpr int kDSPVectorSizeFloat = kProcessChunkSize;
constexpr uintptr_t kfloatsPerSIMDVectorBits = 2;
constexpr uintptr_t kSIMDVecSize = 1 << kfloatsPerSIMDVectorBits;
constexpr int kDSPVectorSizeSIMD = kDSPVectorSizeFloat / kSIMDVecSize;

#include "MLDSPMathSSE.h"

#endif

