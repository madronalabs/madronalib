// MLDSPOps.h
// madronalib
//
// Created by Randy Jones on 4/5/2016
//
// This module contains the DSPVectorArray / DSPVector class and basic operations on it. 
// Any stateless operations on DSPVectors should be added here.
// 
// These objects are for building fixed DSP graphs in a functional style. The compiler should 
// have many opportunities to optimize these graphs. For dynamic graphs changeable at runtime,
// see MLProcs. In general MLProcs will be written using DSPGens, DSPOps, DSPFilters.

#pragma once

#include <array>
#include <functional>
#include <iostream>

#include "../core/MLScalarMath.h"
#include "MLDSPMath.h"
#include "MLDSPConstants.h"

namespace ml
{				
	// constexpr std::array fill machinery
	template <size_t N>
	struct constArrayRecursion 
	{
		template <typename T, typename ...Tn>
		static constexpr auto apply(T v, Tn ...vs)
		-> decltype(constArrayRecursion<N - 1>::apply(v, v, vs...))
		{
			return constArrayRecursion<N - 1>::apply(v, v, vs...);
		}
	};
	
	template <>
	struct constArrayRecursion<1> 
	{
		template <typename T, typename ...Tn>
		static constexpr auto apply(T v, Tn ...vs)
		-> std::array<T, sizeof...(vs) + 1>
		{
			return std::array<T, sizeof...(vs) + 1> {v, vs...};
		}
	};
	
	template <typename T, size_t N>
	struct constArray {
		std::array<T, N> data;
		constexpr constArray(T val) : data(constArrayRecursion<N>::apply(val)) {}
	};

	template<int VECTORS> 
	struct DSPVectorArrayConst
	{
		constArray<float, kFloatsPerDSPVector*VECTORS> constData;		
		constexpr DSPVectorArrayConst(float f) : constData(f) {}
	};
	
	template<int VECTORS>
	class DSPVectorArray
	{
		typedef union 			
		{
			std::array<float, kFloatsPerDSPVector*VECTORS> mArrayData; // for constexpr ctor
			SIMDVectorFloat asVector[kSIMDVectorsPerDSPVector*VECTORS];
			float asFloat[kFloatsPerDSPVector*VECTORS];
		}	DSPVectorData;
 
	public:
		DSPVectorData mData;

		DSPVectorArray() { zero(); }
		explicit DSPVectorArray(float k) { operator=(k); }
		explicit DSPVectorArray(float * pData) { load(*this, pData); }
		constexpr DSPVectorArray(DSPVectorArrayConst<VECTORS> v) : mData{{v.constData.data}} {}
		
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
		
		// TODO a move constructor might help efficiency but is quite different from the current design. 
		// for moving to make sense, the data can't be on the stack. instead of the current stack-based approach
		// there could be a memory pool for all DSPVector data. some kind of allocator would be needed, which
		// would be a pain but then it would be possible to write e.g. DSPVector c = a + b without having to 
		// copy the temporary (a + b) as we do now. 
		//
		// a move ctor should be marked noexcept.
				
		// DSPVectors are always aligned, take advantage of this for fast copying
		inline DSPVectorArray<VECTORS> operator=(const DSPVectorArray<VECTORS>& x1)
		{
			const float* px1 = x1.getConstBuffer();
			float* py1 = getBuffer();
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
			{
				vecStore(py1, vecLoad(px1));
				px1 += kFloatsPerSIMDVector;
				py1 += kFloatsPerSIMDVector;
			}
			return *this;
		}
		
		// return row J from this DSPVectorArray, when J is known at compile time. 
		template<int J>
		inline DSPVectorArray<1> getRowVector() const
		{
			static_assert((J >= 0) && (J < VECTORS), "getRowVector index out of bounds"); 
			return getRowVectorUnchecked(J);
		}		
		
		// set row J of this DSPVectorArray to x1, when J is known at compile time. 
		template<int J>
		inline void setRowVector(const DSPVectorArray<1>& x1)
		{
			static_assert((J >= 0) && (J < VECTORS), "setRowVector index out of bounds");
			setRowVectorUnchecked(J, x1);
		}
			
		// get a row vector j when j is not known at compile time. 
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
		
		// set a row vector j when j is not known at compile time. 
		inline void setRowVectorUnchecked(int j, const DSPVectorArray<1>& x1)
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
		
		// return a pointer to the first element in row J of this DSPVectorArray.
		template<int J>
		inline const float* getRowDataConst() const
		{
			static_assert((J >= 0) && (J < VECTORS), "getRowDataConst row index out of bounds"); 
			const float* py1 = getConstBuffer() + kFloatsPerDSPVector*J;
			return py1;
		}		
						
		inline DSPVectorArray& operator+=(const DSPVectorArray& x1){*this = add(*this, x1); return *this;}
		inline DSPVectorArray& operator-=(const DSPVectorArray& x1){*this = subtract(*this, x1); return *this;}
		inline DSPVectorArray& operator*=(const DSPVectorArray& x1){*this = multiply(*this, x1); return *this;}
		inline DSPVectorArray& operator/=(const DSPVectorArray& x1){*this = divide(*this, x1); return *this;}
		
		// declare as friends any templates or functions that need to use get/setRowVectorUnchecked
		template<int C, int V>
		friend DSPVectorArray<C*V> repeat(const DSPVectorArray<V>& x1);

		template<int VA, int VB>
		friend DSPVectorArray<VA + VB> append(const DSPVectorArray<VA>& x1, const DSPVectorArray<VB>& x2);
	};
	
	
	template<int VECTORS>
	class DSPVectorArrayConstTest : public DSPVectorArray<VECTORS>
	{
	public:
		// DSPVectorArray<VECTORS> data;
		constexpr DSPVectorArrayConstTest(float f) : DSPVectorArray<VECTORS>(DSPVectorArrayConst<VECTORS>(f)){}
	};
	
	typedef DSPVectorArray<1> DSPVector;
	typedef DSPVectorArrayConstTest<1> DSPVectorConst;

	
// ----------------------------------------------------------------
// load and store

	// loads and stores may be unaligned, let std::copy handle this
	template<int VECTORS>
	inline void load(DSPVectorArray<VECTORS>& vecDest, float* pSrc) 
	{ 
		std::copy(pSrc, pSrc + kFloatsPerDSPVector*VECTORS, vecDest.mData.asFloat); 
	}

	template<int VECTORS>
	inline void store(const DSPVectorArray<VECTORS>& vecSrc, float* pDest) 
	{ 
		std::copy(vecSrc.mData.asFloat, vecSrc.mData.asFloat + kFloatsPerDSPVector*VECTORS, pDest); 
	}

// ----------------------------------------------------------------
// unary operators
	
	#define DEFINE_OP1(opName, opComputation)					\
	template<int VECTORS>										\
	inline DSPVectorArray<VECTORS>								\
		(opName)(const DSPVectorArray<VECTORS>& vx1)					\
	{															\
		DSPVectorArray<VECTORS> vy;								\
		const float* px1 = vx1.getConstBuffer();							\
		float* py1 = vy.getBuffer();							\
		for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)		\
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
	DEFINE_OP1(sin, (vecSin(x)));
	DEFINE_OP1(cos, (vecCos(x)));
	DEFINE_OP1(log, (vecLog(x)));
	DEFINE_OP1(exp, (vecExp(x)));
	
	// lazy log2 and exp2 from natural log / exp
	STATIC_M128_CONST(kLogTwoVec, 0.69314718055994529f);
	STATIC_M128_CONST(kLogTwoRVec, 1.4426950408889634f);	
	DEFINE_OP1(log2, (vecMul(vecLog(x), kLogTwoRVec)));
	DEFINE_OP1(exp2, (vecExp(vecMul(kLogTwoVec, x))));

	// trig, log and exp, using polynomial approximations
	DEFINE_OP1(sinApprox, (vecSinApprox(x)));
	DEFINE_OP1(cosApprox, (vecCosApprox(x)));	
	DEFINE_OP1(expApprox, (vecExpApprox(x)));
	DEFINE_OP1(logApprox, (vecLogApprox(x)));

	// lazy log2 and exp2 approximations from log / exp approximations
	DEFINE_OP1(log2Approx, (vecMul(vecLogApprox(x), kLogTwoRVec)));
	DEFINE_OP1(exp2Approx, (vecExpApprox(vecMul(kLogTwoVec, x))));

// ----------------------------------------------------------------
// binary operators
	
	#define DEFINE_OP2(opName, opComputation)					\
	template<int VECTORS>										\
	inline DSPVectorArray<VECTORS>(opName)						\
		(const DSPVectorArray<VECTORS>& vx1,					\
		const DSPVectorArray<VECTORS>& vx2)						\
	{															\
		DSPVectorArray<VECTORS> vy;								\
		const float* px1 = vx1.getConstBuffer();				\
		const float* px2 = vx2.getConstBuffer();				\
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

	inline DSPVector operator+(const DSPVector& x1, const DSPVector& x2){return add(x1, x2);}
	inline DSPVector operator-(const DSPVector& x1, const DSPVector& x2){return subtract(x1, x2);}
	inline DSPVector operator*(const DSPVector& x1, const DSPVector& x2){return multiply(x1, x2);}
	inline DSPVector operator/(const DSPVector& x1, const DSPVector& x2){return divide(x1, x2);}
	
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
	// ternary operators
	
	#define DEFINE_OP3(opName, opComputation)					\
	template<int VECTORS>										\
	inline DSPVectorArray<VECTORS>								\
		(opName)(const DSPVectorArray<VECTORS>& vx1,			\
		const DSPVectorArray<VECTORS>& vx2,						\
		const DSPVectorArray<VECTORS>& vx3)						\
	{															\
		DSPVectorArray<VECTORS> vy;								\
		const float* px1 = vx1.getConstBuffer();				\
		const float* px2 = vx2.getConstBuffer();				\
		const float* px3 = vx3.getConstBuffer();				\
		float* py1 = vy.getBuffer();							\
		for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)		\
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

	DEFINE_OP3(select, vecSelect(x1, x2, x3));					// bitwise select(resultIfTrue, resultIfFalse, conditionMask)
	DEFINE_OP3(lerp, vecAdd(x1, (vecMul(x3, vecSub(x2, x1)))));	// lerp(a, b, mix). NB: "(x1 + (x3 * (x2 - x1)))" would be pretty but does not work on Windows
	DEFINE_OP3(clamp, vecClamp(x1, x2, x3) );					// clamp(x, minBound, maxBound) 
	DEFINE_OP3(within, vecWithin(x1, x2, x3) );					// is x in the open interval [x2, x3) ?
	
	// ----------------------------------------------------------------
	// single-vector index and range generators
	
	inline DSPVector columnIndex()
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
			indexVec = vecAdd(indexVec, stepVec);
		}													
		return vy;											
	}
	
	// return a linear sequence from start to end, where end will fall on the first index of the 
	// next vector.
	inline DSPVector rangeOpen(float start, float end)
	{
		DSPVector vi = columnIndex();
		float interval = (end - start)/(kFloatsPerDSPVector);
		return vi*DSPVector(interval) + DSPVector(start);
	}
	
	// return a linear sequence from start to end, where end falls on the last index of this vector.
	inline DSPVector rangeClosed(float start, float end)
	{
		DSPVector vi = columnIndex();
		float interval = (end - start)/(kFloatsPerDSPVector - 1.f);
		return vi*DSPVector(interval) + DSPVector(start);											
	}
		
	// ----------------------------------------------------------------
	// single-vector horizontal operators returning float
	
	inline float sum(const DSPVector& x)
	{
		const float* px1 = x.getConstBuffer();
		float sum = 0;
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)		
		{
			sum += vecSumH(vecLoad(px1));
			px1 += kFloatsPerSIMDVector;	
		}
		return sum;
	}
	
	inline float mean(const DSPVector& x)
	{
		constexpr float kGain = 1.0f/kFloatsPerDSPVector;
		return sum(x)*kGain;
	}
	
	inline float max(const DSPVector& x)
	{
		const float* px1 = x.getConstBuffer();
		float fmax = FLT_MIN;
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)		
		{
			fmax = ml::max(fmax, vecMaxH(vecLoad(px1)));
			px1 += kFloatsPerSIMDVector;	
		}
		return fmax;
	}
	
	inline float min(const DSPVector& x)
	{
		const float* px1 = x.getConstBuffer();
		float fmin = FLT_MAX;
		for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)		
		{
			fmin = ml::min(fmin, vecMinH(vecLoad(px1)));
			px1 += kFloatsPerSIMDVector;	
		}
		return fmin;
	}
	
	// ----------------------------------------------------------------
	// functional
	
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
	inline DSPVectorArray<VECTORS> map(std::function<DSPVector(const DSPVector&, int)> f, const DSPVectorArray<VECTORS>& x)
	{
		DSPVectorArray<VECTORS> y;
		for(int j=0; j<VECTORS; ++j)
		{
			y.setRowVectorUnchecked(j, f(x.getRowVectorUnchecked(j), j));
		}			
		return y;
	}

	// ----------------------------------------------------------------
	// rowIndex
	
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
	// combining rows
	
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
	// separating rows
	
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
	// integer DSPVector
	
	typedef DSPVectorArray<1> DSPVector;
	
	constexpr int kIntsPerDSPVector = kFloatsPerDSPVector;
	
	template<int VECTORS>
	class DSPVectorArrayInt
	{
		typedef union
		{
			SIMDVectorInt asVector[kSIMDVectorsPerDSPVector*VECTORS];
			int32_t asInt[kFloatsPerDSPVector*VECTORS];
		} DSPVectorData;
		
	public:
		DSPVectorData mData;
		explicit DSPVectorArrayInt() { }
		explicit DSPVectorArrayInt(int32_t k) { operator=(k); }
		
		inline int32_t& operator[](int i) { return mData.asInt[i]; }	
		inline const int32_t operator[](int i) const { return mData.asInt[i]; }			
		inline int32_t* getBufferInt() {return (mData.asInt);} 
		inline const int32_t* getConstBufferInt() const {return (mData.asInt);}
		inline float* getBuffer() {return reinterpret_cast<float*>(mData.asInt);} 
		inline const float* getConstBuffer() const {return reinterpret_cast<const float*>(mData.asInt);}

		// set each element of the DSPVectorArray to the int32_t value k.
		inline DSPVectorArrayInt<VECTORS> operator=(int32_t k)
		{
			const SIMDVectorInt vk = vecSetInt1(k); 	
			int32_t* py1 = getBufferInt();
			
			for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
			{
				vecStore(reinterpret_cast<float*>(py1), (vk));
				py1 += kIntsPerSIMDVector;
			}
			return *this;
		}
	};
	
	typedef DSPVectorArrayInt<1> DSPVectorInt;

	// ----------------------------------------------------------------
	// unary float -> int operators 
		
	#define DEFINE_OP1_F2I(opName, opComputation)				\
	template<int VECTORS>										\
	inline DSPVectorArrayInt<VECTORS>							\
		(opName)(const DSPVectorArray<VECTORS>& vx1)			\
	{															\
		DSPVectorArrayInt<VECTORS> vy;							\
		const float* px1 = vx1.getConstBuffer();				\
		float* py1 = vy.getBuffer();							\
		for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)		\
		{														\
			SIMDVectorFloat x = vecLoad(px1);					\
			vecStore((py1), (opComputation));						\
			px1 += kFloatsPerSIMDVector;						\
			py1 += kIntsPerSIMDVector;							\
		}														\
		return vy;												\
	}	

	DEFINE_OP1_F2I(roundFloatToInt, (vecFloatToIntRound(x)));
	DEFINE_OP1_F2I(truncateFloatToInt, (vecFloatToIntTruncate(x)));

	// ----------------------------------------------------------------
	// unary int -> float operators 
		
	#define DEFINE_OP1_I2F(opName, opComputation)				\
	template<int VECTORS>										\
	inline DSPVectorArray<VECTORS>								\
		(opName)(const DSPVectorArrayInt<VECTORS>& vx1)			\
	{															\
		DSPVectorArray<VECTORS> vy;							\
		const float* px1 = vx1.getConstBuffer();				\
		float* py1 = vy.getBuffer();							\
		for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)		\
		{														\
			SIMDVectorInt x = VecF2I(vecLoad(px1));						\
			vecStore((py1), (opComputation));						\
			px1 += kIntsPerSIMDVector;							\
			py1 += kFloatsPerSIMDVector;						\
		}														\
		return vy;												\
	}	
	
	DEFINE_OP1_I2F(intToFloat, (vecIntToFloat(x)));

	// ----------------------------------------------------------------
	// generators of DSPVector constants
	
	inline DSPVector unityInterpVector()
	{
		DSPVector y;
		float fn = kFloatsPerDSPVector;
		for(int i=0; i < kFloatsPerDSPVector; ++i)
		{
			float fi = i;
			y[i] = fi / fn;
		}
		return y;
	}
	
	// ----------------------------------------------------------------
	// for testing
	
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
	
	template<int VECTORS>
	inline std::ostream& operator<< (std::ostream& out, const DSPVectorArrayInt<VECTORS>& vecArray)
	{
		if(VECTORS > 1) out << "[   ";
		for(int v=0; v<VECTORS; ++v)
		{
			if(VECTORS > 1) if(v > 0) out << "\n    ";
			if(VECTORS > 1) out << "v" << v << ": ";
			out << "[";
			for(int i=0; i<kIntsPerDSPVector; ++i)
			{
				out << vecArray[v*kIntsPerDSPVector + i] << " ";
			}
			out << "] ";
		}
		if(VECTORS > 1) out << "]";
		return out;
	}	
	
	inline bool validate(const DSPVector& x)
	{
		bool r = true;
		for(int n=0; n<kFloatsPerDSPVector; ++n)
		{
			const float maxUsefulValue = 1e8; 
			if(ml::isNaN(x[n]) || (fabs(x[n]) > maxUsefulValue)) 
			{
				std::cout << "error: " << x[n] << " at index " << n << "\n";
				std::cout << x << "\n";
				r = false;
				break;
			}
		}
		return r;
	}	
} // namespace ml


