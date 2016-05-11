// MLDSPOps.h
// madronalib
//
// Created by Randy Jones on 4/5/2016
//
// This module contains the DSPVectorArray / DSPVector class and basic operations on it. 
// Any stateless operations on DSPVectors should be added here.

#pragma once

#include <functional>
#include <iostream>

#include "MLScalarMath.h"
#include "MLDSPMath.h"
#include "MLDSPConstants.h"

namespace ml
{				
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
		inline const float* getConstBuffer() const {return mData.asFloat;}
		
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
		
		inline DSPVectorArray<1> getRowVectorUnchecked(int j) const
		{
			DSPVectorArray<1> vy;
			const float* px1 = getConstBuffer() + kFloatsPerDSPVector*j;
			float* py1 = vy.getBuffer();
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)
			{
				vecStore(py1, vecLoad(px1));
				px1 += kFloatsPerSIMDVector;
				py1 += kFloatsPerSIMDVector;
			}
			return vy;
		}

		inline void setRowVectorUnchecked(int j, DSPVectorArray<1> x1)
		{
			const float* px1 = x1.getConstBuffer();
			float* py1 = getBuffer() + kFloatsPerDSPVector*j;
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)
			{
				vecStore(py1, vecLoad(px1));
				px1 += kFloatsPerSIMDVector;
				py1 += kFloatsPerSIMDVector;
			}	
		}
		
		// return row J from this DSPVectorArray.
		template<int J>
		inline DSPVectorArray<1> getRowVector() const
		{
			static_assert((J >= 0) && (J < VECTORS), "getRowVector index out of bounds"); 
			return getRowVectorUnchecked(J);
		}		
		
		// set row J of this DSPVectorArray to x1.
		template<int J>
		inline void setRowVector(DSPVectorArray<1> x1)
		{
			static_assert((J >= 0) && (J < VECTORS), "setRowVector index out of bounds");
			setRowVectorUnchecked(J, x1);
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
	
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> operator+(DSPVectorArray<VECTORS> x1, DSPVectorArray<VECTORS> x2){return add(x1, x2);}
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> operator-(DSPVectorArray<VECTORS> x1, DSPVectorArray<VECTORS> x2){return subtract(x1, x2);}
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> operator*(DSPVectorArray<VECTORS> x1, DSPVectorArray<VECTORS> x2){return multiply(x1, x2);}
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> operator/(DSPVectorArray<VECTORS> x1, DSPVectorArray<VECTORS> x2){return divide(x1, x2);}
	
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
	#pragma mark single-vector index and range generators
	
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
	#pragma mark single-vector horizontal operators returning float
	
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
	
	// Evaluate a function (void)->(float), store at each element of the DSPVectorArray and return the result.
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

	// Apply a function (float)->(float) to each element of the DSPVectorArray x and return the result.
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
	
	// Apply a function (DSPVector, int row)->(DSPVector) to each row of the DSPVectorArray x and return the result.
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> map(std::function<DSPVector(DSPVector, int)> f, const DSPVectorArray<VECTORS>& x)
	{
		DSPVectorArray<VECTORS> y;
		for(int j=0; j<VECTORS; ++j)
		{
			y.setRowVectorUnchecked(j, f(x.getRowVectorUnchecked(j), j));
		}			
		return y;
	}
	
	// ----------------------------------------------------------------
	#pragma mark rowIndex
	
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> rowIndex()
	{
		DSPVectorArray<VECTORS> y;
		for(int j=0; j<VECTORS; ++j)
		{
			y.setRowVectorUnchecked(j, DSPVector(j));
		}			
		return y;
	}
	
	// ----------------------------------------------------------------
	#pragma mark combining rows
	
	// return a DSPVectorArray with each row set to the single DSPVector x1.
	template<int COPIES, int VECTORS>
	inline DSPVectorArray<COPIES*VECTORS> repeat(const DSPVectorArray<VECTORS>& x1)
	{
		DSPVectorArray<COPIES*VECTORS> vy;
		for(int copy=0; copy<COPIES; ++copy)
		{
			for(int j=0; j<VECTORS; ++j)
			{
				vy.setRowVectorUnchecked(copy*VECTORS + j, x1.getRowVectorUnchecked(j));
			}
		}
		return vy;
	}
	
	template<int VECTORSA, int VECTORSB>
	inline DSPVectorArray<VECTORSA + VECTORSB> append(const DSPVectorArray<VECTORSA>& x1, const DSPVectorArray<VECTORSB>& x2)
	{
		DSPVectorArray<VECTORSA + VECTORSB> vy;
		for(int j=0; j<VECTORSA; ++j)
		{
			vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(j));
		}
		for(int j=VECTORSA; j<VECTORSA + VECTORSB; ++j)
		{
			vy.setRowVectorUnchecked(j, x2.getRowVectorUnchecked(j - VECTORSA));
		}		
		return vy;
	}
	
	// shuffle two DSPVectorArrays, alternating x1 to even rows of result and x2 to odd rows.
	// if the sources are different sizes, the excess rows are all appended to the destination after shuffling is done.
	template<int VECTORSA, int VECTORSB>
	inline DSPVectorArray<VECTORSA + VECTORSB> shuffle(const DSPVectorArray<VECTORSA>& x1, const DSPVectorArray<VECTORSB>& x2)
	{
		DSPVectorArray<VECTORSA + VECTORSB> vy;
		int ja = 0;
		int jb = 0;
		int jy = 0;
		while((ja < VECTORSA) || (jb < VECTORSB))
		{
			if(ja < VECTORSA)
			{
				vy.setRowVectorUnchecked(jy, x1.getRowVectorUnchecked(ja));
				ja++; jy++;
			}
			if(jb < VECTORSB)
			{
				vy.setRowVectorUnchecked(jy, x2.getRowVectorUnchecked(jb));
				jb++; jy++;
			}
		}
		return vy;
	}
	
	// ----------------------------------------------------------------
	#pragma mark separating rows
	
	template<int VECTORS>
	inline DSPVectorArray<(VECTORS + 1)/2> evenRows(const DSPVectorArray<VECTORS>& x1)
	{
		DSPVectorArray<(VECTORS + 1)/2> vy;		
		for(int j=0; j<(VECTORS + 1)/2; ++j)
		{
			vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(j*2));
		}		
		return vy;
	}
	
	template<int VECTORS>
	inline DSPVectorArray<VECTORS/2> oddRows(const DSPVectorArray<VECTORS>& x1)
	{
		DSPVectorArray<VECTORS/2> vy;		
		for(int j=0; j<VECTORS/2; ++j)
		{
			vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(j*2 + 1));
		}		
		return vy;
	}
	
	// ----------------------------------------------------------------
	#pragma mark for testing
	
	template<int VECTORS>
	inline std::ostream& operator<< (std::ostream& out, const DSPVectorArray<VECTORS>& vecArray)
	{
		if(VECTORS > 1) out << "[   ";
		for(int v=0; v<VECTORS; ++v)
		{
			if(VECTORS > 1) if(v > 0) out << "\n    ";
			if(VECTORS > 1) out << "v" << v << ": ";
			out << "[";
			for(int i=0; i<kFloatsPerDSPVector; ++i)
			{
				out << vecArray[v*kFloatsPerDSPVector + i] << " ";
			}
			out << "] ";
		}
		if(VECTORS > 1) out << "]";
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
