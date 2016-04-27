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
// Simple operations on DSPVectorArray.
//
// This module should include any basic operations that have no state. 

namespace ml
{			
	// this template is not specialized for false, which will generate a
	// compile-time error if STATIC_CHECK(expr) is not true.
	template<bool> struct CompileTimeError;
	template<> struct CompileTimeError<true> {}; 	
	#define STATIC_CHECK(expr)  (CompileTimeError<(expr) != 0>())
	
	template<int VECTORS>
	class DSPVectorArray
	{
		typedef union
		{
			SIMDVectorFloat asVector[kSIMDVectorsPerDSPVector*VECTORS];
			float asFloat[kFloatsPerDSPVector*VECTORS];
		} DSPVectorData;

	public:
		DSPVectorData mData;

		DSPVectorArray() { zero(); }
		DSPVectorArray(float k) { operator=(k); }
		
		inline float& operator[](int i) { return mData.asFloat[i]; }	
		inline const float operator[](int i) const { return mData.asFloat[i]; }			
		inline float* getBuffer() {return mData.asFloat;}
		
		// set each element of the DSPVectorArray to 0.
		inline DSPVectorArray<VECTORS> zero()
		{
			float* py1 = getBuffer();			
			for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
			{
				vecStore(py1, vecZeros());
				py1 += kFloatsPerSIMDVector;
			}
			return *this;
		}
		
		// set each element of the DSPVectorArray to the float value k.
		inline DSPVectorArray<VECTORS> operator=(float k)
		{
			const SIMDVectorFloat vk = vecSet1(k); 	
			float* py1 = getBuffer();
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
			{
				vecStore(py1, vk);
				py1 += kFloatsPerSIMDVector;
			}
			return *this;
		}
		
		// DSPVectors are always aligned, take advantage of this for fast copying
		inline DSPVectorArray<VECTORS> operator=(DSPVectorArray<VECTORS> x1)
		{
			float* px1 = x1.getBuffer();
			float* py1 = getBuffer();
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
			{
				vecStore(py1, vecLoad(px1));
				px1 += kFloatsPerSIMDVector;
				py1 += kFloatsPerSIMDVector;
			}
			return *this;
		}

		// ----------------------------------------------------------------
		#pragma fill
		
		// set each DSPVector of this DSPVectorArray to the single DSPVector x1.
		inline DSPVectorArray<VECTORS> fill(DSPVectorArray<1> x1)
		{
			for(int j=0; j<VECTORS; ++j)
			{
				float* px1 = x1.getBuffer();
				float* py1 = getBuffer() + kFloatsPerDSPVector*j;
				
				for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)
				{
					vecStore(py1, vecLoad(px1));
					px1 += kFloatsPerSIMDVector;
					py1 += kFloatsPerSIMDVector;
				}
			}
			return *this;
		}

		// return a single DSPVector fomr this DSPVectorArray.
		template<int I>
		inline DSPVectorArray<1> getVector()
		{
			STATIC_CHECK((I >= 0) && (I < VECTORS)); 
			
			DSPVectorArray<1> vy;
			float* px1 = getBuffer() + kFloatsPerDSPVector*I;
			float* py1 = vy.getBuffer();
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)
			{
				vecStore(py1, vecLoad(px1));
				px1 += kFloatsPerSIMDVector;
				py1 += kFloatsPerSIMDVector;
			}
			return vy;
		}		
		
		// set a single DSPVector of this DSPVectorArray to x1.
		template<int I>
		inline DSPVectorArray<VECTORS> setVector(DSPVectorArray<1> x1)
		{
			STATIC_CHECK((I >= 0) && (I < VECTORS));
			
			float* px1 = x1.getBuffer();
			float* py1 = getBuffer() + kFloatsPerDSPVector*I;
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)
			{
				vecStore(py1, vecLoad(px1));
				px1 += kFloatsPerSIMDVector;
				py1 += kFloatsPerSIMDVector;
			}
			return *this;
		}
		
		inline DSPVectorArray& operator+=(DSPVectorArray x1){*this = add(*this, x1); return *this;}
		inline DSPVectorArray& operator-=(DSPVectorArray x1){*this = subtract(*this, x1); return *this;}
		inline DSPVectorArray& operator*=(DSPVectorArray x1){*this = multiply(*this, x1); return *this;}
		inline DSPVectorArray& operator/=(DSPVectorArray x1){*this = divide(*this, x1); return *this;}
	};

	typedef DSPVectorArray<1> DSPVector;
	
	// ----------------------------------------------------------------
	#pragma mark load and store

	// loads and stores may be unaligned, let std::copy handle this
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> load(float* pSrc) 
	{ 
		DSPVectorArray<VECTORS> v;
		std::copy(pSrc, pSrc + kFloatsPerDSPVector*VECTORS, v.mData.asFloat); 
		return v;
	}

	template<int VECTORS>
	inline void store(const DSPVectorArray<VECTORS>& vecSrc, float* pDest) 
	{ std::copy(vecSrc.mData.asFloat, vecSrc.mData.asFloat + kFloatsPerDSPVector*VECTORS, pDest); }

	// ----------------------------------------------------------------
	#pragma mark unary operators
	
	#define DEFINE_OP1(opName, opComputation)					\
	template<int VECTORS>										\
	inline DSPVectorArray<VECTORS>								\
		(opName)(DSPVectorArray<VECTORS> vx1)					\
	{															\
		DSPVectorArray<VECTORS> vy;								\
		float* px1 = vx1.getBuffer();							\
		float* py1 = vy.getBuffer();							\
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)		\
		{														\
			SIMDVectorFloat x = vecLoad(px1);					\
			vecStore(py1, (opComputation));						\
			px1 += kFloatsPerSIMDVector;						\
			py1 += kFloatsPerSIMDVector;						\
		}														\
		return vy;												\
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
	
	#define DEFINE_OP2(opName, opComputation)					\
	template<int VECTORS>										\
	inline DSPVectorArray<VECTORS>(opName)						\
		(DSPVectorArray<VECTORS> vx1,							\
		DSPVectorArray<VECTORS> vx2)							\
	{															\
		DSPVectorArray<VECTORS> vy;								\
		float* px1 = vx1.getBuffer();							\
		float* px2 = vx2.getBuffer();							\
		float* py1 = vy.getBuffer();							\
		for (int n = 0;											\
			n < kSIMDVectorsPerDSPVector*VECTORS; ++n)			\
		{														\
			SIMDVectorFloat x1 = vecLoad(px1);					\
			SIMDVectorFloat x2 = vecLoad(px2);					\
			vecStore(py1, (opComputation));						\
			px1 += kFloatsPerSIMDVector;						\
			px2 += kFloatsPerSIMDVector;						\
			py1 += kFloatsPerSIMDVector;						\
		}														\
		return vy;												\
	}															\

	DEFINE_OP2(add, (vecAdd(x1, x2)));
	DEFINE_OP2(subtract, (vecSub(x1, x2)));
	DEFINE_OP2(multiply, (vecMul(x1, x2)));
	DEFINE_OP2(divide, (vecDiv(x1, x2)));

	inline DSPVector operator+(DSPVector x1, DSPVector x2){return add(x1, x2);}
	inline DSPVector operator-(DSPVector x1, DSPVector x2){return subtract(x1, x2);}
	inline DSPVector operator*(DSPVector x1, DSPVector x2){return multiply(x1, x2);}
	inline DSPVector operator/(DSPVector x1, DSPVector x2){return divide(x1, x2);}

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

	#define DEFINE_OP3(opName, opComputation)					\
	template<int VECTORS>										\
	inline DSPVectorArray<VECTORS>								\
		(opName)(DSPVectorArray<VECTORS> vx1,					\
		DSPVectorArray<VECTORS> vx2,							\
		DSPVectorArray<VECTORS> vx3)							\
	{															\
		DSPVectorArray<VECTORS> vy;								\
		float* px1 = vx1.getBuffer();							\
		float* px2 = vx2.getBuffer();							\
		float* px3 = vx3.getBuffer();							\
		float* py1 = vy.getBuffer();							\
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)		\
		{														\
			SIMDVectorFloat x1 = vecLoad(px1);					\
			SIMDVectorFloat x2 = vecLoad(px2);					\
			SIMDVectorFloat x3 = vecLoad(px3);					\
			vecStore(py1, (opComputation));						\
			px1 += kFloatsPerSIMDVector;						\
			px2 += kFloatsPerSIMDVector;						\
			px3 += kFloatsPerSIMDVector;						\
			py1 += kFloatsPerSIMDVector;						\
		}														\
		return vy;												\
	}	

	DEFINE_OP3(select, vecSelect(x1, x2, x3));					// conditionMask, resultIfTrue, resultIfFalse
	DEFINE_OP3(lerp, vecAdd(x1, vecMul(x3, vecAdd(x2, x1))));	// x1 + x3*(x2 - x1)
	DEFINE_OP3(clamp, vecClamp(x1, x2, x3) );					// clamp(x, minBound, maxBound) 
	DEFINE_OP3(within, vecWithin(x1, x2, x3) );					// is x in the open interval [x2, x3) ?
	
	// ----------------------------------------------------------------
	#pragma mark index and range generators
	
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
	#pragma mark functional
	
	// Apply a function (float)->(float) to each element of the DSPvector and return the result.
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> map(std::function<float(float)> f, const DSPVectorArray<VECTORS>& x)
	{
		DSPVectorArray<VECTORS> y;
		for(int n=0; n<kFloatsPerDSPVector*VECTORS; ++n)
		{
			y[n] = f(x[n]);
		}
		return y;
	}
	
	// Evaluate a function (void)->(float), store at each element of the DSPvector and return the result.
	// x is a dummy argument just used to infer the vector size.
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> map(std::function<float()> f, const DSPVectorArray<VECTORS>& x)
	{
		DSPVectorArray<VECTORS> y;
		for(int n=0; n<kFloatsPerDSPVector*VECTORS; ++n)
		{
			y[n] = f();
		}
		return y;
	}
	
	// ----------------------------------------------------------------
	#pragma mark for testing
	
	template<int VECTORS>
	inline std::ostream& operator<< (std::ostream& out, const DSPVectorArray<VECTORS>& vecArray)
	{
		out << "[   ";
		for(int v=0; v<VECTORS; ++v)
		{
			if(v > 0) out << "\n    ";
			out << "v" << v << ": ";
			out << "[";
			for(int i=0; i<kFloatsPerDSPVector; ++i)
			{
				out << vecArray[v*kFloatsPerDSPVector + i] << " ";
			}
			out << "] ";
		}
		out << "]\n";
		return out;
	}	
	
	inline bool validate(DSPVector x)
	{
		bool r = true;
		for(int n=0; n<kFloatsPerDSPVector; ++n)
		{
			const float maxUsefulValue = 1e8; 
			if(isNaN(x[n]) || (fabs(x[n]) > maxUsefulValue)) 
			{
				std::cout << "error: " << x[n] << " at index " << n << "\n";
				std::cout << x << "\n";
				r = false;
				break;
			}
		}
		return r;
	}
}
