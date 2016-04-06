//
//  MLVectorOps.h
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#ifndef __MLVectorOps__
#define __MLVectorOps__

#include "MLDSP.h"


// ----------------------------------------------------------------
// DSP utility objects -- some very basic building blocks, not in MLProcs
// so they can be used more easily in MLProcs and elsewhere.
//
// This modules should include any DSP functors that we typically want to use from 
// C++ code directly. They will typically be used to implement MLProcs, which can
// be a lot more complicated. 
//
// DSPUtils:
// - are stateless functions if they don't need a sampling rate or memory (add, multiply, etc)
// - are functors if they need a sampling rate or memory (filters, oscillators etc)
// - should be inlined
// - loops should be fixed-sized at compile time and thereby unrollable
// - should use static binding of operator()
// - should output a single DSPVector from operator()(const DSPVector& in1 ...)
// - may need a sample rate to be set
// - may have static data such as tables, created using a
//	singleton pattern when the first object is made
// - do not require any other infrastructure
//
// TODO we can make some of these qualities explicit in the code with a templated base class 
// and CRTP https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// to overload operator() (DSPVector) for each proc class
// base class will have operator()<T> for each proc.
// 
// TODO this is all a statement of intent to refactor things—most
// of the functions here are not yet in line with the guidelines above
//
// for dynamically dispatched objects, making graphs from JSON, etc, see MLProcs.

// TODO test pass by value everywhere idea: 
// - look for difference in generated code of pass by value vs. pointer. 
// - write one fairly complex object with both value and pointer, compare and time.

// best idea so far?
// DSPutils can operate on the assumption of default size signals: 16 x 1 or whatever.
// and, if making default signals is fast enough (TIME IT) we can return new default sigs by value.
// these can coexist with slower matrix-like MLSignal methods that actually do range checking. 


// ----------------------------------------------------------------
#pragma mark DSPVectors

namespace ml
{	
	constexpr int kDSPVectorSizeFloat = kMLProcessChunkSize;
	constexpr int kDSPVectorSizeSSE = kDSPVectorSizeFloat / kSSEVecSize;
	
	typedef union
	{
		__m128 asVector[kDSPVectorSizeSSE];
		float asFloat[kDSPVectorSizeFloat];
	} DSPVectorData;
	
	class DSPVector
	{
	private:
		DSPVectorData mData;
	public:
		DSPVector() {}
		const DSPVector(const float* pf) { copyFrom(pf); }
		
		inline float& operator[](int i) { return mData.asFloat[i]; }	
		inline const float operator[](int i) const { return mData.asFloat[i]; }	
		
		// TEMP glue to MLSignal-based graphs
		inline void copyFrom(const float* pSrc) { std::copy(pSrc, pSrc+kDSPVectorSizeFloat, mData.asFloat); } 
		inline void copyTo (float* pDest) { std::copy(mData.asFloat, mData.asFloat+kDSPVectorSizeFloat, pDest); }
		
		void setToConstant(float k);
	};


	// ----------------------------------------------------------------
	#pragma mark stateless functions

	inline DSPVector add(const DSPVector& x1, const DSPVector& x2)
	{
		DSPVector y;

		for(int n=0; n < ml::kDSPVectorSizeFloat; ++n)
		{
			y[n] = x1[n] + x2[n]; // TODO SSE
		}
		
		/*
		 int c = frames >> kMLSamplesPerSSEVectorBits;
		 __m128 vx1, vx2, vr; 	

		 for (int n = 0; n < c; ++n)
		 {
		 vx1 = _mm_load_ps(px1);
		 vx2 = _mm_load_ps(px2);
		 vr = _mm_add_ps(vx1, vx2);
		 _mm_store_ps(py1, vr);
		 px1 += kSSEVecSize;
		 px2 += kSSEVecSize;
		 py1 += kSSEVecSize;
		 }
	*/
		return y;
	}

}

#endif /* defined(__MLVectorOps__) */
