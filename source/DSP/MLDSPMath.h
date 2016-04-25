//
//  MLDSPMath.h
//  madronalib
//
//  Created by Randy Jones on 4/14/2016
//
//

#pragma once

// Here is the global process chunk size, an important constant.
constexpr int kFloatsPerDSPVector = 64;

// Load definitions for low-level SIMD math. 
// These must define SIMDVectorFloat, SIMDVectorInt, their sizes, and a bunch of operations on them.
// We are currently only using 4-element vectors on both SSE and NEON.

#if(defined  __ARM_NEON) || (defined __ARM_NEON__)

// NEON 

#include "MLDSPMathNEON.h"

#else 

// SSE2

#include "MLDSPMathSSE.h"

#endif

/*
 MLVec MLVec::getIntPart() const
 {
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	return MLVec(_mm_cvtepi32_ps(vi));
 }
 
 MLVec MLVec::getFracPart() const
 {
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	__m128 intPart = _mm_cvtepi32_ps(vi);
	return MLVec(_mm_sub_ps(val.v, intPart));
 }
 
 void MLVec::getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const
 {
	__m128i vi = _mm_cvttps_epi32(val.v);	// convert with truncate
	intPart = _mm_cvtepi32_ps(vi);
	fracPart = _mm_sub_ps(val.v, intPart.val.v);
 }
*/