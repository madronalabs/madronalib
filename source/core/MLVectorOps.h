//
//  MLVectorOps.h
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#pragma once

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
// - should output a single DSPVector from operator()(DSPVector in1 ...)
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
// - look for difference in generated code of pass by value vs. pointer.  OK 
// - write one fairly complex object with both value and pointer, compare and time.

// best idea so far?
// DSPutils can operate on the assumption of default size signals: 16 x 1 or whatever.
// and, if making default signals is fast enough (TIME IT) we can return new default sigs by value.
// these can coexist with slower matrix-like MLSignal methods that actually do range checking. 


// ----------------------------------------------------------------
#pragma mark DSPVectors

namespace ml
{		
	class DSPVector;
	
	constexpr int kDSPVectorSizeFloat = kMLProcessChunkSize;
	constexpr int kDSPVectorSizeSSE = kDSPVectorSizeFloat / kSSEVecSize;
	
	typedef union
	{
		__m128 asVector[kDSPVectorSizeSSE];
		float asFloat[kDSPVectorSizeFloat];
	} DSPVectorData;
	
	// prototypes for math functions used in member operators
	inline DSPVector add(DSPVector x1, DSPVector x2);
	inline DSPVector subtract(DSPVector x1, DSPVector x2);
	inline DSPVector multiply(DSPVector x1, DSPVector x2);
	inline DSPVector divide(DSPVector x1, DSPVector x2);
	
	class DSPVector
	{
	private:
		DSPVectorData mData;
	public:
		// NOTE for efficiency the default ctor does not zero the data!
		DSPVector() {}
		DSPVector(float k) { operator=(k); }
		
		inline float& operator[](int i) { return mData.asFloat[i]; }	
		inline const float operator[](int i) const { return mData.asFloat[i]; }			
		inline float* getBuffer() {return mData.asFloat;}
		
		// TEMP glue to MLSignal-based graphs
	//	inline void copyFrom(const float* pSrc) { std::copy(pSrc, pSrc+kDSPVectorSizeFloat, mData.asFloat); } 
	//	inline void copyTo (float* pDest) { std::copy(mData.asFloat, mData.asFloat+kDSPVectorSizeFloat, pDest); }
		
		inline void operator=(float k)
		{
			const __m128 vk = _mm_set1_ps(k); 	
			float* py1 = getBuffer();
			
			for (int n = 0; n < kDSPVectorSizeSSE; ++n)
			{
				_mm_store_ps(py1, vk);
				py1 += kSSEVecSize;
			}
		}
		inline void operator=(DSPVector x1)
		{
			float* px1 = x1.getBuffer();
			float* py1 = getBuffer();
			
			for (int n = 0; n < kDSPVectorSizeSSE; ++n)
			{
				_mm_store_ps(py1, _mm_load_ps(px1));
				px1 += kSSEVecSize;
				py1 += kSSEVecSize;
			}
		}
		inline DSPVector operator+(DSPVector x1){return add(*this, x1);}
		inline DSPVector operator-(DSPVector x1){return subtract(*this, x1);}
		inline DSPVector operator*(DSPVector x1){return multiply(*this, x1);}
		inline DSPVector operator/(DSPVector x1){return divide(*this, x1);}
		inline void operator+=(DSPVector x1){*this = add(*this, x1);}
		inline void operator-=(DSPVector x1){*this = subtract(*this, x1);}
		inline void operator*=(DSPVector x1){*this = multiply(*this, x1);}
		inline void operator/=(DSPVector x1){*this = divide(*this, x1);}
	};

	// ----------------------------------------------------------------
	#pragma mark unary operators
	
	#define DEFINE_OP1(opName, opComputation)				\
	inline DSPVector (opName)(DSPVector x1)					\
	{														\
		DSPVector y;										\
		float* px1 = x1.getBuffer();						\
		float* py1 = y.getBuffer();							\
		for (int n = 0; n < kDSPVectorSizeSSE; ++n)			\
		{													\
			__m128 x = _mm_load_ps(px1);					\
			_mm_store_ps(py1, (opComputation));				\
			px1 += kSSEVecSize;								\
			py1 += kSSEVecSize;								\
		}													\
		return y;											\
	}	

	DEFINE_OP1(sqrt, (_mm_sqrt_ps(x)));
	DEFINE_OP1(sqrtApprox, (_mm_mul_ps(x, _mm_rsqrt_ps(x))));
	DEFINE_OP1(abs, (_mm_andnot_ps(_mm_set_ps1(-0.0f), x)));
	DEFINE_OP1(sign, 
			   (_mm_and_ps(
					(_mm_or_ps(_mm_and_ps(_mm_set_ps1(-0.0f), x), _mm_set_ps1(1.0f))),
				   _mm_cmpneq_ps(_mm_set_ps1(-0.0f), x)
				))
			   );
	
	
	
	// ----------------------------------------------------------------
	#pragma mark binary operators
	
	#define DEFINE_OP2(opName, opComputation)				\
	inline DSPVector (opName)(DSPVector x1, DSPVector x2)	\
	{														\
		DSPVector y;										\
		float* px1 = x1.getBuffer();						\
		float* px2 = x2.getBuffer();						\
		float* py1 = y.getBuffer();							\
		for (int n = 0; n < kDSPVectorSizeSSE; ++n)			\
		{													\
			__m128 x1 = _mm_load_ps(px1);					\
			__m128 x2 = _mm_load_ps(px2);					\
			_mm_store_ps(py1, (opComputation));				\
			px1 += kSSEVecSize;								\
			px2 += kSSEVecSize;								\
			py1 += kSSEVecSize;								\
		}													\
		return y;											\
		}	
	
	DEFINE_OP2(add, (_mm_add_ps(x1, x2)));
	DEFINE_OP2(subtract, (_mm_sub_ps(x1, x2)));
	DEFINE_OP2(multiply, (_mm_mul_ps(x1, x2)));
	DEFINE_OP2(divide, (_mm_div_ps(x1, x2)));
	DEFINE_OP2(divideApprox, (_mm_mul_ps(x1, _mm_rcp_ps(x2))));
	DEFINE_OP2(min, (_mm_min_ps(x1, x2)));
	DEFINE_OP2(max, (_mm_max_ps(x1, x2)));

	
	/*
	 Vector Ops
	 =======
	 
	 unary:
	 sin / approx / approx2
	 cos / approx / approx2
	 exp2 / approx / approx2
	 log2 / approx / approx2
	 floatSign
	 saturateBounded
	 softclip
	 
	 binary:
	 pow / approx / approx2
	 
	
	 ternary:
	 lerp
	 clamp
	 within
	 
	 
	 4-op:
	 lerp3
	 
	 utils (functors)
	 -----------
	 
	 0 operands (generators):
	 RandomSource
	 sineOsc
	 TriOsc
	 PhaseOsc
	 
	 1 operand
	 differentiator
	 integrator
	 SampleDelay 
	 LinearDelay
	 AllpassDelay (or, interp. set by function? allpass interp. has state. )	 
	 FDN	 
	 Downsampler2
	 upsampler2
	 
	 inline DSPVector SVF::operator();
	 biquad
	 onepole
	 asymmetriconepole
	 
	 ramp generator
	 quadratic generator
	 
	 
	 banks:
	 ----
	 sinebank
	 phasebank
	 SVFbank
	 biquadbank
	 
	 
	*/
	
	
	
}

std::ostream& operator<< (std::ostream& out, const ml::DSPVector& v);
