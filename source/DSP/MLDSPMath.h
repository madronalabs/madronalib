//
//  MLDSPMath.h
//  madronalib
//
//  Created by Randy Jones on 4/14/2016
//
// 

#pragma once

// Here is the DSP vector size, an important constant. 
constexpr int kFloatsPerDSPVector = 64;

// Load definitions for low-level SIMD math. 
// These must define SIMDVectorFloat, SIMDVectorInt, their sizes, and a bunch of operations on them.
// We are currently only using 4-element vectors on both SSE and NEON.

#if(defined  __ARM_NEON) || (defined __ARM_NEON__)

// NEON 

// TODO
#include "MLDSPMathNEON.h"

#else 

// SSE2

#include "MLDSPMathSSE.h"

#endif
