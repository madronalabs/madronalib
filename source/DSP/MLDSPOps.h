//
//  MLDSPOps.h
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#pragma once

#include <functional>
#include <iostream>

#include "MLScalarMath.h"
#include "MLDSPMath.h"
#include "MLDSPConstants.h"

// ----------------------------------------------------------------
// Simple operations on DSPVectors.
//
// This module should include any DSP operations that have no state. 

namespace ml
{		
	typedef union
	{
		SIMDVectorFloat asVector[kSIMDVectorsPerDSPVector];
		float asFloat[kFloatsPerDSPVector];
	} DSPVectorData;
	
	// prototypes for math functions used in member operators
	class DSPVector;
	inline DSPVector add(DSPVector x1, DSPVector x2);
	inline DSPVector subtract(DSPVector x1, DSPVector x2);
	inline DSPVector multiply(DSPVector x1, DSPVector x2);
	inline DSPVector divide(DSPVector x1, DSPVector x2);
	
	class DSPVector
	{
	public:
		DSPVectorData mData;

		// NOTE for efficiency the default ctor does not zero the data!
		DSPVector() {}
		DSPVector(float k) { operator=(k); }
		
		inline float& operator[](int i) { return mData.asFloat[i]; }	
		inline const float operator[](int i) const { return mData.asFloat[i]; }			
		inline float* getBuffer() {return mData.asFloat;}
		
		inline void operator=(float k)
		{
			const SIMDVectorFloat vk = vecSet1(k); 	
			float* py1 = getBuffer();
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)
			{
				vecStore(py1, vk);
				py1 += kFloatsPerSIMDVector;
			}
		}
		inline void operator=(DSPVector x1)
		{
			float* px1 = x1.getBuffer();
			float* py1 = getBuffer();
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)
			{
				vecStore(py1, vecLoad(px1));
				px1 += kFloatsPerSIMDVector;
				py1 += kFloatsPerSIMDVector;
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
	#pragma mark index and ranges
	
	inline DSPVector index()
	{
		static constexpr SIMDVectorFloat intsVec = {0, 1, 2, 3};
		static constexpr SIMDVectorFloat stepVec = {4, 4, 4, 4};
		DSPVector vy;																
		float* py1 = vy.getBuffer();	
		SIMDVectorFloat indexVec = intsVec;
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)			
		{													
			vecStore(py1, indexVec);											
			py1 += kFloatsPerSIMDVector;		
			indexVec += stepVec;
		}													
		return vy;											
	}
	
	// return a linear sequence from start to end, where end will fall on the first index of the 
	// next vector.
	inline DSPVector rangeOpen(float start, float end)
	{
		DSPVector vi = index();
		float interval = (end - start)/(kFloatsPerDSPVector);
		return vi*interval + start;											
	}
	
	// return a linear sequence from start to end, where end falls on the last index of this vector.
	inline DSPVector rangeClosed(float start, float end)
	{
		DSPVector vi = index();
		float interval = (end - start)/(kFloatsPerDSPVector - 1.f);
		return vi*interval + start;											
	}
	
	// ----------------------------------------------------------------
	#pragma mark unary operators
	
	#define DEFINE_OP1(opName, opComputation)				\
	inline DSPVector (opName)(DSPVector vx1)				\
	{														\
		DSPVector vy;										\
		float* px1 = vx1.getBuffer();						\
		float* py1 = vy.getBuffer();						\
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)	\
		{													\
			SIMDVectorFloat x = vecLoad(px1);				\
			vecStore(py1, (opComputation));					\
			px1 += kFloatsPerSIMDVector;					\
			py1 += kFloatsPerSIMDVector;					\
		}													\
		return vy;											\
	}	

	DEFINE_OP1(sqrt, (vecSqrt(x)));
	DEFINE_OP1(sqrtApprox, vecSqrtApprox(x));
	DEFINE_OP1(abs, vecAbs(x));
	
	// float sign: -1, 0, or 1
	DEFINE_OP1(sign, vecSign(x));
	
	// up/down sign: -1 or 1
	DEFINE_OP1(signBit, vecSignBit(x));
	
	// trig, log and exp, using accurate cephes-derived library
	DEFINE_OP1(cos, (vecCos(x)));
	DEFINE_OP1(sin, (vecSin(x)));
	DEFINE_OP1(log, (vecLog(x)));
	DEFINE_OP1(exp, (vecExp(x)));
	
	// lazy log2 and exp2 from natural log / exp
	STATIC_M128_CONST(kLogTwoVec, 0.69314718055994529f);
	STATIC_M128_CONST(kLogTwoRVec, 1.4426950408889634f);	
	DEFINE_OP1(vecLog2, (vecMul(vecLog(x), kLogTwoRVec)));
	DEFINE_OP1(vecExp2, (vecExp(vecMul(kLogTwoVec, x))));

	// trig, log and exp, using polynomial approximations
	DEFINE_OP1(sinApprox, (vecSinApprox(x)));
	DEFINE_OP1(cosApprox, (vecCosApprox(x)));	
	DEFINE_OP1(expApprox, (vecExpApprox(x)));
	DEFINE_OP1(logApprox, (vecLogApprox(x)));

	// lazy log2 and exp2 approximations from log / exp approximations
	DEFINE_OP1(log2Approx, (vecMul(vecLogApprox(x), kLogTwoRVec)));
	DEFINE_OP1(exp2Approx, (vecExpApprox(vecMul(kLogTwoVec, x))));

	// ----------------------------------------------------------------
	#pragma mark binary operators
	
	#define DEFINE_OP2(opName, opComputation)				\
	inline DSPVector (opName)(DSPVector vx1, DSPVector vx2)	\
	{														\
		DSPVector vy;										\
		float* px1 = vx1.getBuffer();						\
		float* px2 = vx2.getBuffer();						\
		float* py1 = vy.getBuffer();						\
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)	\
		{													\
			SIMDVectorFloat x1 = vecLoad(px1);				\
			SIMDVectorFloat x2 = vecLoad(px2);				\
			vecStore(py1, (opComputation));					\
			px1 += kFloatsPerSIMDVector;					\
			px2 += kFloatsPerSIMDVector;					\
			py1 += kFloatsPerSIMDVector;					\
		}													\
		return vy;											\
	}	
	
	DEFINE_OP2(add, (vecAdd(x1, x2)));
	DEFINE_OP2(subtract, (vecSub(x1, x2)));
	DEFINE_OP2(multiply, (vecMul(x1, x2)));
	DEFINE_OP2(divide, (vecDiv(x1, x2)));
	DEFINE_OP2(divideApprox, vecDivApprox(x1, x2) );
	DEFINE_OP2(pow, (vecExp(vecMul(vecLog(x1), x2))));
	DEFINE_OP2(powApprox, (vecExpApprox(vecMul(vecLogApprox(x1), x2))));
	DEFINE_OP2(min, (vecMin(x1, x2)));
	DEFINE_OP2(max, (vecMax(x1, x2)));

	DEFINE_OP2(equal, (vecEqual(x1, x2)));
	DEFINE_OP2(notEqual, (vecNotEqual(x1, x2)));
	DEFINE_OP2(greaterThan, (vecGreaterThan(x1, x2)));
	DEFINE_OP2(greaterThanOrEqual, (vecGreaterThanOrEqual(x1, x2)));
	DEFINE_OP2(lessThan, (vecLessThan(x1, x2)));
	DEFINE_OP2(lessThanOrEqual, (vecLessThanOrEqual(x1, x2)));

	// ----------------------------------------------------------------
	#pragma mark ternary operators
	
	#define DEFINE_OP3(opName, opComputation)				\
	inline DSPVector (opName)(DSPVector vx1,				\
		DSPVector vx2, DSPVector vx3)						\
	{														\
		DSPVector vy;										\
		float* px1 = vx1.getBuffer();						\
		float* px2 = vx2.getBuffer();						\
		float* px3 = vx3.getBuffer();						\
		float* py1 = vy.getBuffer();						\
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)	\
		{													\
			SIMDVectorFloat x1 = vecLoad(px1);				\
			SIMDVectorFloat x2 = vecLoad(px2);				\
			SIMDVectorFloat x3 = vecLoad(px3);				\
			vecStore(py1, (opComputation));					\
			px1 += kFloatsPerSIMDVector;					\
			px2 += kFloatsPerSIMDVector;					\
			px3 += kFloatsPerSIMDVector;					\
			py1 += kFloatsPerSIMDVector;					\
		}													\
		return vy;											\
	}	

	DEFINE_OP3(select, vecSelect(x1, x2, x3));
	DEFINE_OP3(lerp, vecAdd(x1, vecMul(x3, vecAdd(x2, x1)))); // x1 + x3*(x2 - x1)
	DEFINE_OP3(clamp, vecClamp(x1, x2, x3) ); // clamp(x, minBound, maxBound) 
	
	// within (x, lowerBound, upperBound)
	// is x in the open interval [x2, x3) ?
	// returns a bitmask of all ones when true: note that as float this result is a NaN.
	DEFINE_OP3(within, vecWithin(x1, x2, x3) );
	
	// ----------------------------------------------------------------
	#pragma mark horizontal operators returning float
	
	inline float sum(DSPVector x)
	{
		float* px1 = x.getBuffer();
		float sum = 0;
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)		
		{
			sum += vecSumH(vecLoad(px1));
			px1 += kFloatsPerSIMDVector;	
		}
		return sum;
	}
	
	inline float mean(DSPVector x)
	{
		constexpr float kGain = 1.0f/kFloatsPerDSPVector;
		return sum(x)*kGain;
	}
	
	inline float max(DSPVector x)
	{
		float* px1 = x.getBuffer();
		float fmax = FLT_MIN;
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)		
		{
			fmax = ml::max(fmax, vecMaxH(vecLoad(px1)));
			px1 += kFloatsPerSIMDVector;	
		}
		return fmax;
	}
	
	inline float min(DSPVector x)
	{
		float* px1 = x.getBuffer();
		float fmin = FLT_MAX;
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)		
		{
			fmin = ml::min(fmin, vecMinH(vecLoad(px1)));
			px1 += kFloatsPerSIMDVector;	
		}
		return fmin;
	}
	
	// ----------------------------------------------------------------
	#pragma mark functions concerning other functions
	
	// Apply a function (float)->(float) to each element of the DSPvector and return the result.
	inline DSPVector map(std::function<float(float)> f, DSPVector x)
	{
		DSPVector y;
		for(int n=0; n<kFloatsPerDSPVector; ++n)
		{
			y[n] = f(x[n]);
		}
		return y;
	}

	// fill the vector with successive runs of a function returning float.
	inline DSPVector fill(std::function<float()> f)
	{
		DSPVector y;
		for(int n=0; n<kFloatsPerDSPVector; ++n)
		{
			y[n] = f();
		}
		return y;
	}
}

std::ostream& operator<< (std::ostream& out, const ml::DSPVector& v);
