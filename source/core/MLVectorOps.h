//
//  MLVectorOps.h
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#pragma once

#include "MLDSP.h"
#include "MLMathCephesSSE.h"

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

	#define STATIC_M128_CONST(name, val) \
	static constexpr __m128 name = {val, val, val, val};		
	
	inline __m128 select_ps(__m128 conditionMask, __m128 a, __m128 b)
	{
		__m128i zero = _mm_setzero_si128();
		__m128i ones = _mm_cmpeq_epi32(zero, zero);
		return _mm_or_ps(
						 _mm_and_ps(conditionMask, a),
						 _mm_and_ps(_mm_andnot_ps(conditionMask, ones), b)
						 );
	}

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
	
	// float sign: -1, 0, or 1
	DEFINE_OP1(sign, 
			   (_mm_and_ps
				(
				 _mm_or_ps(_mm_and_ps(_mm_set_ps1(-0.0f), x), _mm_set_ps1(1.0f)),
				 _mm_cmpneq_ps(_mm_set_ps1(-0.0f), x)
				 )
				)
			   );
	
	// up/down sign: -1 or 1
	DEFINE_OP1(signBit, _mm_or_ps(_mm_and_ps(_mm_set_ps1(-0.0f), x), _mm_set_ps1(1.0f)));
	
	// trig, using accurate cephes-derived library
	DEFINE_OP1(cos, (cos_ps(x)));
	DEFINE_OP1(sin, (sin_ps(x)));
	
	// log and exp, using accurate cephes-derived library
	DEFINE_OP1(log, (log_ps(x)));
	DEFINE_OP1(exp, (exp_ps(x)));
	
	// lazy log2 and exp2 from natural log / exp
	STATIC_M128_CONST(kLogTwoVec, 0.69314718055994529f);
	STATIC_M128_CONST(kLogTwoRVec, 1.4426950408889634f);	
	DEFINE_OP1(log2, (_mm_mul_ps(log_ps(x), kLogTwoRVec)));
	DEFINE_OP1(exp2, (exp_ps(_mm_mul_ps(kLogTwoVec, x))));

	// fast polynomial approximations 
	// from scalar code by Jacques-Henri Jourdan <jourgun@gmail.com>
	// sin and cos valid from -pi to pi
	// exp and log polynomials generated using Sollya http://sollya.gforge.inria.fr/
	/* Generated in Sollya with:
	 > f=remez(1-x*exp(-(x-1)*log(2)),
	 [|1,(x-1)*(x-2), (x-1)*(x-2)*x, (x-1)*(x-2)*x*x|],
	 [1,2], exp(-(x-1)*log(2)));
	 > plot(exp((x-1)*log(2))/(f+x)-1, [1,2]);
	 > f+x;
  */
	
	
	/* log Generated in Sollya using :
	 > f = remez(log(x)-(x-1)*log(2),
	 [|1,(x-1)*(x-2), (x-1)*(x-2)*x, (x-1)*(x-2)*x*x,
	 (x-1)*(x-2)*x*x*x|], [1,2], 1, 1e-8);
	 > plot(f+(x-1)*log(2)-log(x), [1,2]);
	 > f+(x-1)*log(2)
	 */
	
	STATIC_M128_CONST(kSinC1Vec, 0.99997937679290771484375f);
	STATIC_M128_CONST(kSinC2Vec, -0.166624367237091064453125f);
	STATIC_M128_CONST(kSinC3Vec, 8.30897875130176544189453125e-3f);
	STATIC_M128_CONST(kSinC4Vec, -1.92649182281456887722015380859375e-4f);
	STATIC_M128_CONST(kSinC5Vec, 2.147840177713078446686267852783203125e-6f);
		
	inline __m128 sinapprox_ps(__m128 x) 
	{
		__m128 x2 = _mm_mul_ps(x, x);
		return _mm_mul_ps(x,
				_mm_add_ps(kSinC1Vec, _mm_mul_ps(x2,
				_mm_add_ps(kSinC2Vec, _mm_mul_ps(x2, 
				_mm_add_ps(kSinC3Vec, _mm_mul_ps(x2, 
				_mm_add_ps(kSinC4Vec, _mm_mul_ps(x2, kSinC5Vec)) )) )) )) );
	}
	DEFINE_OP1(sinApprox, (sinapprox_ps(x)));

	STATIC_M128_CONST(kCosC1Vec, 0.999959766864776611328125f);
	STATIC_M128_CONST(kCosC2Vec, -0.4997930824756622314453125f);
	STATIC_M128_CONST(kCosC3Vec, 4.1496001183986663818359375e-2f);
	STATIC_M128_CONST(kCosC4Vec, -1.33926304988563060760498046875e-3f);
	STATIC_M128_CONST(kCosC5Vec, 1.8791708498611114919185638427734375e-5f);

	inline __m128 cosapprox_ps(__m128 x) 
	{
		__m128 x2 = _mm_mul_ps(x, x);
		return _mm_add_ps(kCosC1Vec, _mm_mul_ps(x2,
		_mm_add_ps(kCosC2Vec, _mm_mul_ps(x2, 
		_mm_add_ps(kCosC3Vec, _mm_mul_ps(x2, 
		_mm_add_ps(kCosC4Vec, _mm_mul_ps(x2, kCosC5Vec)) )) )) ));
	}
	DEFINE_OP1(cosApprox, (cosapprox_ps(x)));
	
	STATIC_M128_CONST(kExpC1Vec, 2139095040.f);
	STATIC_M128_CONST(kExpC2Vec, 12102203.1615614f);
	STATIC_M128_CONST(kExpC3Vec, 1065353216.f);
	STATIC_M128_CONST(kExpC4Vec, 0.510397365625862338668154f);
	STATIC_M128_CONST(kExpC5Vec, 0.310670891004095530771135f);
	STATIC_M128_CONST(kExpC6Vec, 0.168143436463395944830000f);
	STATIC_M128_CONST(kExpC7Vec, -2.88093587581985443087955e-3f);
	STATIC_M128_CONST(kExpC8Vec, 1.3671023382430374383648148e-2f);
	
	/* Relative error bounded by 1e-5 for normalized outputs
	 Returns invalid outputs for nan inputs
	 Continuous error */
	inline __m128 expapprox_ps(__m128 x) 
	{
		const __m128 kZeroVec = _mm_setzero_ps();
		
		union { __m128i vi; __m128 vf; } xu, xu2;
		__m128 val2, val3, val4, b;
		__m128i val4i;
		
		val2 = _mm_add_ps(_mm_mul_ps(x, kExpC2Vec), kExpC3Vec);
		val3 = _mm_min_ps(val2, kExpC1Vec);
		val4 = _mm_max_ps(val3, kZeroVec);
		val4i = _mm_cvttps_epi32(val4);
		xu.vi = _mm_and_ps(val4i, _mm_set1_epi32(0x7F800000)); 
		xu2.vi = _mm_or_ps(_mm_and_ps(val4i, _mm_set1_epi32(0x7FFFFF)), _mm_set1_epi32(0x3F800000));
		b = xu2.vf;
		
		return _mm_mul_ps(xu.vf,(
		_mm_add_ps(kExpC4Vec, _mm_mul_ps(b,
		_mm_add_ps(kExpC5Vec, _mm_mul_ps(b, 
		_mm_add_ps(kExpC6Vec, _mm_mul_ps(b, 
		_mm_add_ps(kExpC7Vec, 
				   _mm_mul_ps(b, kExpC8Vec)) )) )) )) ));	
	}
	
	DEFINE_OP1(expApprox, (expapprox_ps(x)));

	STATIC_M128_CONST(kLogC1Vec, -89.970756366f);
	STATIC_M128_CONST(kLogC2Vec, 3.529304993f);
	STATIC_M128_CONST(kLogC3Vec, -2.461222105f);
	STATIC_M128_CONST(kLogC4Vec, 1.130626167f);
	STATIC_M128_CONST(kLogC5Vec, -0.288739945f);
	STATIC_M128_CONST(kLogC6Vec, 3.110401639e-2f);
	STATIC_M128_CONST(kLogC7Vec, 0.69314718055995f);
	
	inline __m128 logapprox_ps(__m128 val) 
	{
		__m128i vZero = _mm_setzero_si128();
		union { __m128i vi; __m128 vf; } valu;

		__m128 exp, addcst, x;
		
		valu.vf = val;
		
		exp = _mm_srli_epi32(valu.vi, 23);//exp = valu.vi >> 23;
			
		/* 89.970756366f = 127 * log(2) - constant term of polynomial */
		
		addcst = select_ps(_mm_cmpge_ps(val, vZero), kLogC1Vec, _mm_set1_ps(-(float)INFINITY));
		
		return addcst;
		/*		valu.i = (valu.i & 0x7FFFFF) | 0x3F800000;
		x = valu.f;
		

  return
		x * (kLogC2Vec + x * (kLogC3Vec +
								 x * (kLogC4Vec + x * (kLogC5Vec +
														  x * kLogC6Vec))))
		+ (addcst + kLogC7Vec*exp);
	}*/
	
	}
	
	
	
	
#if 0
	/* Absolute error bounded by 1e-6 for normalized inputs
	 Returns a finite number for +inf input
	 Returns -inf for nan and <= 0 inputs.
	 Continuous error. */
	inline float logapprox(float val) 
	{
  union { float f; int i; } valu;
  float exp, addcst, x;
  valu.f = val;
  exp = valu.i >> 23;
  /* 89.970756366f = 127 * log(2) - constant term of polynomial */
  addcst = val > 0 ? -89.970756366f : -(float)INFINITY;
  valu.i = (valu.i & 0x7FFFFF) | 0x3F800000;
  x = valu.f;
		

  /* Generated in Sollya using :
   > f = remez(log(x)-(x-1)*log(2),
   [|1,(x-1)*(x-2), (x-1)*(x-2)*x, (x-1)*(x-2)*x*x,
   (x-1)*(x-2)*x*x*x|], [1,2], 1, 1e-8);
   > plot(f+(x-1)*log(2)-log(x), [1,2]);
   > f+(x-1)*log(2)
   */
  return
		x * (3.529304993f + x * (-2.461222105f +
		x * (1.130626167f + x * (-0.288739945f +
		x * 3.110401639e-2f))))
		+ (addcst + 0.69314718055995f*exp);
	}
	
}


#endif

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
	DEFINE_OP2(equal, (_mm_cmpeq_ps(x1, x2)));
	DEFINE_OP2(notEqual, (_mm_cmpneq_ps(x1, x2)));
	DEFINE_OP2(greaterThan, (_mm_cmpgt_ps(x1, x2)));
	DEFINE_OP2(greaterThanOrEqual, (_mm_cmpge_ps(x1, x2)));
	DEFINE_OP2(lessThan, (_mm_cmplt_ps(x1, x2)));
	DEFINE_OP2(lessThanOrEqual, (_mm_cmple_ps(x1, x2)));
	DEFINE_OP2(eitherIsNaN, (_mm_cmpunord_ps(x1, x2)));


	// ----------------------------------------------------------------
	#pragma mark ternary operators
	
	#define DEFINE_OP3(opName, opComputation)				\
	inline DSPVector (opName)(DSPVector x1, DSPVector x2, DSPVector x3)	\
	{														\
		DSPVector y;										\
		float* px1 = x1.getBuffer();						\
		float* px2 = x2.getBuffer();						\
		float* px3 = x3.getBuffer();						\
		float* py1 = y.getBuffer();							\
		for (int n = 0; n < kDSPVectorSizeSSE; ++n)			\
		{													\
			__m128 x1 = _mm_load_ps(px1);					\
			__m128 x2 = _mm_load_ps(px2);					\
			__m128 x3 = _mm_load_ps(px3);					\
			_mm_store_ps(py1, (opComputation));				\
			px1 += kSSEVecSize;								\
			px2 += kSSEVecSize;								\
			px3 += kSSEVecSize;								\
			py1 += kSSEVecSize;								\
		}													\
		return y;											\
		}	


	DEFINE_OP3(select, select_ps(x1, x2, x3));

	
	/*
	 Vector Ops
	 =======	 
	 
	 binary:
	 pow / approx / approx2
	 
	 x^y = exp(y * log(x))
	 
	
	 ternary:
	 lerp
	 clamp
	 within
	 select (if(cond) then a else b) 
	 
	 4-op:
	 lerp3
	 
	 Vector Gens
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

/*
inline float hadd(const vector4f& rhs)
{
#if SSE_INSTR_SET >= 3 // SSE3
	__m128 tmp0 = _mm_hadd_ps(rhs,rhs);
	__m128 tmp1 = _mm_hadd_ps(tmp0,tmp0);
#else
	__m128 tmp0 = _mm_add_ps(rhs,_mm_movehl_ps(rhs,rhs));
	__m128 tmp1 = _mm_add_ss(tmp0,_mm_shuffle_ps(tmp0,tmp0,1));
#endif
	return _mm_cvtss_f32(tmp1);
}

*/
	
	
}

std::ostream& operator<< (std::ostream& out, const ml::DSPVector& v);
