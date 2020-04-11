// MLDSPOps.h
// madronalib
//
// Created by Randy Jones on 4/5/2016
//
// This module contains the DSPVectorArray / DSPVector class and basic operations on it.
// Any stateless operations on DSPVectors should also be added here.
//
// These objects are for building fixed DSP graphs in a functional style. The compiler should
// have many opportunities to optimize these graphs. For dynamic graphs changeable at runtime,
// see MLProcs. In general MLProcs will be written using DSPGens, DSPOps, DSPFilters.

//#pragma once

#ifndef __ML_DSP_OPS__
#define __ML_DSP_OPS__


#ifdef _WIN32
#include <memory>
#else
//#include <tr1/memory>
#endif

#ifdef __INTEL_COMPILER
#include <mathimf.h>
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif


#include <array>
#include <iostream>
#include <functional>

#include "MLScalarMath.h"
#include "MLDSPMath.h"

#if(_WIN32 && (!_WIN64))
  #define MANUAL_ALIGN_DSPVECTOR
#endif

namespace ml
{
  // constexpr index generators
  template<unsigned... Is> struct indexSeq {};
  template<unsigned N, unsigned... Is>
  struct genSequence : genSequence<N - 1, N - 1, Is...> {};
  template<unsigned... Is>
  struct genSequence<0, Is...> : indexSeq<Is...> {};

  // fill std::array using indices
  template<int VECTORS>
  struct DSPVectorArrayData
  {
    std::array<float, kFloatsPerDSPVector*VECTORS> data;
  };

  template<int VECTORS, typename FuncType, unsigned... Is>
  constexpr DSPVectorArrayData<VECTORS> DSPVectorArrayIter(indexSeq<Is...>, FuncType func)
  {
    return{{{ func(Is)... }}};
  }

  template<int VECTORS>
  struct DSPVectorArrayDataInt
  {
    std::array<int, kFloatsPerDSPVector*VECTORS> data;
  };

  template<int VECTORS, typename FuncType, unsigned... Is>
  constexpr DSPVectorArrayDataInt<VECTORS> DSPVectorArrayIterInt(indexSeq<Is...>, FuncType func)
  {
    return{{{ func(Is)... }}};
  }

#ifdef MANUAL_ALIGN_DSPVECTOR
  // it seems unlikely that the constexpr ctors can be made to compile with manual alignment,
  // so we give up on constexpr for win32 api.
  #define ConstDSPVector static DSPVector
  #define ConstDSPVectorArray static DSPVectorArray

  const uintptr_t kDSPVectorAlignBytes = 16;
  const uintptr_t kDSPVectorAlignFloats = kDSPVectorAlignBytes / sizeof(float);
  const uintptr_t kDSPVectorAlignInts = kDSPVectorAlignBytes / sizeof(int);
  const uintptr_t kDSPVectorBytesAlignMask = ~(kDSPVectorAlignBytes - 1);

  inline float* DSPVectorAlignFloatPointer(const float* p)
  {
    uintptr_t pM = (uintptr_t)p;
    pM += (uintptr_t)(ml::kDSPVectorAlignBytes - 1);
    pM &= ml::kDSPVectorBytesAlignMask;
    return reinterpret_cast<float*>(pM);
  }

  inline int* DSPVectorAlignIntPointer(const int* p)
  {
    uintptr_t pM = (uintptr_t)p;
    pM += (uintptr_t)(ml::kDSPVectorAlignBytes - 1);
    pM &= ml::kDSPVectorBytesAlignMask;
    return reinterpret_cast<int*>(pM);
  }

#else
  #define ConstDSPVector constexpr DSPVector
  #define ConstDSPVectorArray constexpr DSPVectorArray
#endif

  template<int VECTORS>
  class DSPVectorArray
  {
    // union def'n
#ifdef MANUAL_ALIGN_DSPVECTOR
    union _Data
    {
      std::array<float, kFloatsPerDSPVector*VECTORS + kDSPVectorAlignFloats> mArrayData; // for constexpr ctor
      float asFloat[kFloatsPerDSPVector*VECTORS + kDSPVectorAlignFloats];

      _Data() {}

      _Data(std::array<float, kFloatsPerDSPVector*VECTORS> a)
      {
        float *py = DSPVectorAlignFloatPointer(this->asFloat);
        for(int i=0; i<kFloatsPerDSPVector*VECTORS; ++i)
        {
          py[i] = a[i];
        }
      }
    };
#else
    union _Data
    {
      SIMDVectorFloat _align[kSIMDVectorsPerDSPVector*VECTORS]; // unused except to force alignment
      std::array<float, kFloatsPerDSPVector*VECTORS> mArrayData; // for constexpr ctor
      float asFloat[kFloatsPerDSPVector*VECTORS];

      _Data() {}
      constexpr _Data(std::array<float, kFloatsPerDSPVector*VECTORS> a) : mArrayData(a) {}
    };

#endif

    _Data mData;

  public:

    // getBuffer, getConstBuffer, DSPVectorArrayData<VECTORS> ctor
#ifdef MANUAL_ALIGN_DSPVECTOR
    inline float* getBuffer() const { return DSPVectorAlignFloatPointer(mData.asFloat); }
    inline const float* getConstBuffer() const { return DSPVectorAlignFloatPointer(mData.asFloat); }

    DSPVectorArray(DSPVectorArrayData<VECTORS> v)
    {
      float *py = DSPVectorAlignFloatPointer(this->mData.asFloat);
      for (int i = 0; i<kFloatsPerDSPVector*VECTORS; ++i)
      {
        py[i] = v.data[i];
      }
    }
#else
    inline float* getBuffer() { return mData.asFloat; }
    inline const float* getConstBuffer() const { return mData.asFloat; }
    constexpr DSPVectorArray(DSPVectorArrayData<VECTORS> v) : mData(v.data) {}

#endif // MANUAL_ALIGN_DSPVECTOR

    // ctor taking an int->float function
#ifdef MANUAL_ALIGN_DSPVECTOR
    DSPVectorArray(float(*fn)(int))
    {
      float *py = DSPVectorAlignFloatPointer(mData.asFloat);
      for (int i = 0; i<kFloatsPerDSPVector*VECTORS; ++i)
      {
        py[i] = fn(i);
      }
    }
#else
    // template for a function to fill a DSPVector at compile time. usage example:
    // float myFillFn(int i) { return i*0.3f; }
    // constexpr DSPVector v(_fillerFn(myFillFn));
    // where myFillFn takes some args and returns float.
    // unfortunately this will not work with a lambda in C++11.
    template<int FVECTORS, typename FuncType>
    constexpr DSPVectorArrayData<VECTORS> _fillerFn(FuncType func) const
    {
      return DSPVectorArrayIter<VECTORS>(genSequence<kFloatsPerDSPVector*FVECTORS>{}, func);
    }

    // sugar for less verbose constexpr ctor using (int -> float) function
    constexpr DSPVectorArray(float(*fn)(int)) : DSPVectorArray(_fillerFn<VECTORS>(fn)) {}
#endif

    // default ctor: zero
    DSPVectorArray()
    {
      mData.mArrayData.fill(0.f);
    }

    // This constructor is not marked explicit, and thereby allows promoting floats to DSPVectors
    // silently. This keeps the syntax of common DSP code shorter: "va + DSPVector(1.f)" becomes
    // just "va + 1.f". TODO create some functions like DSPVector::operator+(float) if that's a win.
    DSPVectorArray(float k) { operator=(k); }

    // unaligned data * ctors
    explicit DSPVectorArray(float * pData) { load(*this, pData); }
    explicit DSPVectorArray(const float * pData) { load(*this, pData); }

    // aligned data * ctors
    explicit DSPVectorArray(DSPVectorArray * pData) { loadAligned(*this, pData); }
    explicit DSPVectorArray(const DSPVectorArray * pData) { loadAligned(*this, pData); }

    inline float& operator[](int i) { return getBuffer()[i]; }
    inline const float operator[](int i) const { return getConstBuffer()[i]; }

    // = float: set each element of the DSPVectorArray to the float value k.
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

    // set each element of the DSPVectorArray to 0.
    inline DSPVectorArray<VECTORS> zero()
    {
      return operator=(0.f);
    }

    // TODO a move constructor might help efficiency but is quite different from the current design.
    // for moving to make sense, the data can't be on the stack. instead of the current stack-based approach
    // there could be a memory pool for all DSPVector data. some kind of allocator would be needed, which
    // would be a pain but then it would be possible to write e.g. DSPVector c = a + b without having to
    // copy the temporary (a + b) as we do now.
    // a move ctor should be marked noexcept.

    // copy ctor and copy assignment operator
#ifdef MANUAL_ALIGN_DSPVECTOR
    DSPVectorArray<VECTORS> (const DSPVectorArray<VECTORS>& x1)
    {
      const float* px1 = x1.getConstBuffer();
      float* py1 = getBuffer();
      for (int n = 0; n < kFloatsPerDSPVector*VECTORS; ++n)
      {
        py1[n] = px1[n];
      }
    }
    DSPVectorArray<VECTORS>& operator=(const DSPVectorArray<VECTORS>& x1)
    {
      const float* px1 = x1.getConstBuffer();
      float* py1 = getBuffer();
      for (int n = 0; n < kFloatsPerDSPVector*VECTORS; ++n)
      {
        py1[n] = px1[n];
      }
      return *this;
    }
#else
    DSPVectorArray<VECTORS>(const DSPVectorArray<VECTORS>& x1)
    {
      const float* px1 = x1.getConstBuffer();
      float* py1 = getBuffer();

      for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
      {
        vecStore(py1, vecLoad(px1));
        px1 += kFloatsPerSIMDVector;
        py1 += kFloatsPerSIMDVector;
      }
    }
    DSPVectorArray<VECTORS>& operator=(const DSPVectorArray<VECTORS>& x1)
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

#endif

    // equality by value
    bool operator==(const DSPVectorArray<VECTORS>& x1)
    {
      const float* px1 = x1.getConstBuffer();
      const float* py1 = getConstBuffer();

      for (int n = 0; n < kFloatsPerDSPVector*VECTORS; ++n)
      {
        if(py1[n] != px1[n]) return false;
      }
      return true;
    }

  private:

    // return row J from this DSPVectorArray, when J is known at compile time.
    template<int J>
    inline DSPVectorArray<1> getRowVector() const
    {
      static_assert((J >= 0) && (J < VECTORS), "getRowVector index out of bounds");
      return getRowVectorUnchecked(J);
    }

    // set row J of this DSPVectorArray to x1, when J is known at compile time.
    template<int J>
    inline void setRowVector(const DSPVectorArray<1> x1)
    {
      static_assert((J >= 0) && (J < VECTORS), "setRowVector index out of bounds");
      setRowVectorUnchecked(J, x1);
    }


#ifdef MANUAL_ALIGN_DSPVECTOR
    // get a row vector j when j is not known at compile time.
    inline DSPVectorArray<1> getRowVectorUnchecked(int j) const
    {
      DSPVectorArray<1> vy;
      const float* px1 = getConstBuffer() + kFloatsPerDSPVector*j;
      float* py1 = vy.getBuffer();

      for (int n = 0; n < kFloatsPerDSPVector; ++n)
      {
        py1[n] = px1[n];
      }
      return vy;
    }

    // set a row vector j when j is not known at compile time.
    inline void setRowVectorUnchecked(int j, const DSPVectorArray<1> x1)
    {
      const float* px1 = x1.getConstBuffer();
      float* py1 = getBuffer() + kFloatsPerDSPVector*j;

      for (int n = 0; n < kFloatsPerDSPVector; ++n)
      {
        py1[n] = px1[n];
      }
    }
#else
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
    inline void setRowVectorUnchecked(int j, const DSPVectorArray<1> x1)
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
#endif

    // return a const pointer to the first element in row J of this DSPVectorArray.
    inline const float* getRowDataConst(int j) const
    {
      const float* py1 = getConstBuffer() + kFloatsPerDSPVector*j;
      return py1;
    }

    // return a pointer to the first element in row J of this DSPVectorArray.
    inline float* getRowData(int j)
    {
      float* py1 = getBuffer() + kFloatsPerDSPVector*j;
      return py1;
    }

  public:

    // return a reference to a row of this DSPVectorArray.
    inline DSPVectorArray<1>& row(int j)
    {
      float* py1 = getBuffer() + kFloatsPerDSPVector*j;
      DSPVectorArray<1>* pRow = reinterpret_cast<DSPVectorArray<1>*>(py1);
      return *pRow;
    }

    // return a reference to a row of this DSPVectorArray.
    inline const DSPVectorArray<1>& constRow(int j) const
    {
      const float* py1 = getConstBuffer() + kFloatsPerDSPVector*j;
      const DSPVectorArray<1>* pRow = reinterpret_cast<const DSPVectorArray<1>*>(py1);
      return *pRow;
    }

    inline DSPVectorArray& operator+=(const DSPVectorArray& x1){*this = add(*this, x1); return *this;}
    inline DSPVectorArray& operator-=(const DSPVectorArray& x1){*this = subtract(*this, x1); return *this;}
    inline DSPVectorArray& operator*=(const DSPVectorArray& x1){*this = multiply(*this, x1); return *this;}
    inline DSPVectorArray& operator/=(const DSPVectorArray& x1){*this = divide(*this, x1); return *this;}

    // declare as friends any templates or functions that need to use get/setRowVectorUnchecked
    // but maybe they should just use row()?
    template<int C, int V>
    friend DSPVectorArray<C*V> repeat(const DSPVectorArray<V> x1);

    template<int VA, int VB>
    friend DSPVectorArray<VA + VB> append(const DSPVectorArray<VA> x1, const DSPVectorArray<VB> x2);
  }; // class DSPVectorArray

  typedef DSPVectorArray<1> DSPVector;

  // ----------------------------------------------------------------
  // integer DSPVector
  constexpr int kIntsPerDSPVector = kFloatsPerDSPVector;

  template<int VECTORS>
  class DSPVectorArrayInt
  {

#ifdef MANUAL_ALIGN_DSPVECTOR
    union _Data
    {
      std::array<int, kIntsPerDSPVector*VECTORS + kDSPVectorAlignInts> mArrayData;
      int32_t asInt[kIntsPerDSPVector*VECTORS + kDSPVectorAlignInts];
      float asFloat[kFloatsPerDSPVector*VECTORS + kDSPVectorAlignFloats];

      _Data() {}

      _Data(std::array<int, kIntsPerDSPVector*VECTORS> a)
      {
        float *py = DSPVectorAlignIntPointer(this->asFloat);
        for (int i = 0; i < kIntsPerDSPVector*VECTORS; ++i)
        {
          py[i] = a[i];
        }
      }
    };
#else
    union _Data
    {
      SIMDVectorInt _align[kSIMDVectorsPerDSPVector*VECTORS]; // unused except to force alignment
      std::array<int32_t, kIntsPerDSPVector*VECTORS> mArrayData; // for constexpr ctor
      int32_t asInt[kIntsPerDSPVector*VECTORS];
      float asFloat[kFloatsPerDSPVector*VECTORS];

      _Data() {}
      constexpr _Data(std::array<int32_t, kIntsPerDSPVector*VECTORS> a) : mArrayData(a) {}
    };
#endif // MANUAL_ALIGN_DSPVECTOR

  public:
    _Data mData;

    // getBuffer, getConstBuffer, DSPVectorArrayData<VECTORS> ctor
#ifdef MANUAL_ALIGN_DSPVECTOR
    inline float* getBuffer() const { return DSPVectorAlignFloatPointer(mData.asFloat); }
    inline const float* getConstBuffer() const { return DSPVectorAlignFloatPointer(mData.asFloat); }
    inline int32_t* getBufferInt() const { return DSPVectorAlignIntPointer(mData.asInt); }
    inline const int32_t* getConstBufferInt() const { return DSPVectorAlignIntPointer(mData.asInt); }

    DSPVectorArrayInt(DSPVectorArrayData<VECTORS> v)
    {
      float *py = DSPVectorAlignFloatPointer(this->mData.asFloat);
      for (int i = 0; i<kFloatsPerDSPVector*VECTORS; ++i)
      {
        py[i] = v.data[i];
      }
    }
#else
    inline float* getBuffer() { return mData.asFloat; }
    inline const float* getConstBuffer() const { return mData.asFloat; }
    inline int32_t* getBufferInt() { return mData.asInt; }
    inline const int32_t* getConstBufferInt() const { return mData.asInt; }
    constexpr DSPVectorArrayInt(DSPVectorArrayDataInt<VECTORS> v) : mData(v.data) {}
#endif // MANUAL_ALIGN_DSPVECTOR


    explicit DSPVectorArrayInt() { operator=(0); }
    explicit DSPVectorArrayInt(int32_t k) { operator=(k); }

    inline int32_t& operator[](int i) { return getBufferInt()[i]; }
    inline const int32_t operator[](int i) const { return getConstBufferInt()[i]; }

    // set each element of the DSPVectorArray to the int32_t value k.
    inline DSPVectorArrayInt<VECTORS> operator=(int32_t k)
    {
      SIMDVectorFloat vk = VecI2F(vecSetInt1(k));
      int32_t* py1 = getBufferInt();

      for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
      {
        vecStore(reinterpret_cast<float*>(py1), vk);
        py1 += kIntsPerSIMDVector;
      }
      return *this;
    }

    // ctor taking an int->int function
#ifdef MANUAL_ALIGN_DSPVECTOR
    DSPVectorArrayInt(int(*fn)(int))
    {
      int *py = DSPVectorAlignIntPointer(mData.asInt);
      for (int i = 0; i<kFloatsPerDSPVector*VECTORS; ++i)
      {
        py[i] = fn(i);
      }
    }
#else
    // template for a function to fill a DSPVector at compile time. usage example:
    // int myFillFn(int i) { return i/2; }
    // constexpr DSPVectorInt v(_fillerFn(myFillFn));
    // where myFillFn takes some args and returns int.
    // unfortunately this will not work with a lambda in C++11.
    template<int FVECTORS, typename FuncType>
    constexpr DSPVectorArrayDataInt<VECTORS> _fillerFn(FuncType func) const
    {
      return DSPVectorArrayIterInt<VECTORS>(genSequence<kFloatsPerDSPVector*FVECTORS>{}, func);
    }

    // sugar for less verbose constexpr ctor using (int -> int) function
    constexpr DSPVectorArrayInt(int(*fn)(int)) : DSPVectorArrayInt(_fillerFn<VECTORS>(fn)) {}
#endif

    // copy ctor and copy assignment operator
#ifdef MANUAL_ALIGN_DSPVECTOR
    DSPVectorArrayInt<VECTORS>(const DSPVectorArrayInt<VECTORS>& x1)
    {
      const int* px1 = x1.getConstBufferInt();
      int* py1 = getBufferInt();
      for (int n = 0; n < kIntsPerDSPVector*VECTORS; ++n)
      {
        py1[n] = px1[n];
      }
    }
    DSPVectorArrayInt<VECTORS>& operator=(const DSPVectorArrayInt<VECTORS>& x1)
    {
      const int* px1 = x1.getConstBufferInt();
      int* py1 = getBufferInt();
      for (int n = 0; n < kIntsPerDSPVector*VECTORS; ++n)
      {
        py1[n] = px1[n];
      }
      return *this;
    }
#else
    DSPVectorArrayInt<VECTORS>(const DSPVectorArrayInt<VECTORS>& x1)
    {
      const float* px1 = x1.getConstBuffer();
      float* py1 = getBuffer();
      for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
      {
        vecStore(py1, vecLoad(px1));
        px1 += kFloatsPerSIMDVector;
        py1 += kFloatsPerSIMDVector;
      }
    }
    inline DSPVectorArrayInt<VECTORS>& operator=(const DSPVectorArrayInt<VECTORS>& x1)
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
#endif

    // equality by value
    bool operator==(const DSPVectorArrayInt<VECTORS>& x1)
    {
      const int* px1 = x1.getConstBufferInt();
      const int* py1 = getConstBufferInt();

      for (int n = 0; n < kIntsPerDSPVector*VECTORS; ++n)
      {
        if(py1[n] != px1[n]) return false;
      }
      return true;
    }


  }; // class DSPVectorArrayInt

  typedef DSPVectorArrayInt<1> DSPVectorInt;

// ----------------------------------------------------------------
// load and store

  // some loads and stores may be unaligned, let std::copy handle this
  template<int VECTORS>
  inline void load(DSPVectorArray<VECTORS>& vecDest, const float* pSrc)
  {
    std::copy(pSrc, pSrc + kFloatsPerDSPVector*VECTORS, vecDest.getBuffer());
  }

  template<int VECTORS>
  inline void store(const DSPVectorArray<VECTORS>& vecSrc, float* pDest)
  {
    std::copy(vecSrc.getConstBuffer(), vecSrc.getConstBuffer() + kFloatsPerDSPVector*VECTORS, pDest);
  }

  // if the pointers are known to be aligned, copy as SIMD vectors
  template<int VECTORS>
  inline void loadAligned(DSPVectorArray<VECTORS>& vecDest, const float* pSrc)
  {
    const float* px1 = pSrc;
    float* py1 = vecDest.getBuffer();

    for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
    {
      vecStore(py1, vecLoad(px1));
      px1 += kFloatsPerSIMDVector;
      py1 += kFloatsPerSIMDVector;
    }
  }

  template<int VECTORS>
  inline void storeAligned(const DSPVectorArray<VECTORS>& vecSrc, float* pDest)
  {
    const float* px1 = vecSrc.getConstBuffer();
    float* py1 = pDest;

    for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)
    {
      vecStore(py1, vecLoad(px1));
      px1 += kFloatsPerSIMDVector;
      py1 += kFloatsPerSIMDVector;
    }
  }

// ----------------------------------------------------------------
// unary operators

  #define DEFINE_OP1(opName, opComputation)          \
  template<int VECTORS>                    \
  inline DSPVectorArray<VECTORS>                \
    (opName)(const DSPVectorArray<VECTORS>& vx1)          \
  {                              \
    DSPVectorArray<VECTORS> vy;                \
    const float* px1 = vx1.getConstBuffer();              \
    float* py1 = vy.getBuffer();              \
    for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)    \
    {                            \
      SIMDVectorFloat x = vecLoad(px1);          \
      vecStore(py1, (opComputation));            \
      px1 += kFloatsPerSIMDVector;            \
      py1 += kFloatsPerSIMDVector;            \
    }                            \
    return vy;                        \
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

  #define DEFINE_OP2(opName, opComputation)          \
  template<int VECTORS>                    \
  inline DSPVectorArray<VECTORS>(opName)            \
    (const DSPVectorArray<VECTORS>& vx1,          \
    const DSPVectorArray<VECTORS>& vx2)            \
  {                              \
    DSPVectorArray<VECTORS> vy;                \
    const float* px1 = vx1.getConstBuffer();        \
    const float* px2 = vx2.getConstBuffer();        \
    float* py1 = vy.getBuffer();              \
    for (int n = 0;                      \
      n < kSIMDVectorsPerDSPVector*VECTORS; ++n)      \
    {                            \
      SIMDVectorFloat x1 = vecLoad(px1);          \
      SIMDVectorFloat x2 = vecLoad(px2);          \
      vecStore(py1, (opComputation));            \
      px1 += kFloatsPerSIMDVector;            \
      px2 += kFloatsPerSIMDVector;            \
      py1 += kFloatsPerSIMDVector;            \
    }                            \
    return vy;                        \
  }                              \

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

  // ----------------------------------------------------------------
  // ternary operators

  #define DEFINE_OP3(opName, opComputation)          \
  template<int VECTORS>                    \
  inline DSPVectorArray<VECTORS>                \
    (opName)(const DSPVectorArray<VECTORS>& vx1,      \
    const DSPVectorArray<VECTORS>& vx2,            \
    const DSPVectorArray<VECTORS>& vx3)            \
  {                              \
    DSPVectorArray<VECTORS> vy;                \
    const float* px1 = vx1.getConstBuffer();        \
    const float* px2 = vx2.getConstBuffer();        \
    const float* px3 = vx3.getConstBuffer();        \
    float* py1 = vy.getBuffer();              \
    for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)    \
    {                            \
      SIMDVectorFloat x1 = vecLoad(px1);          \
      SIMDVectorFloat x2 = vecLoad(px2);          \
      SIMDVectorFloat x3 = vecLoad(px3);          \
      vecStore(py1, (opComputation));            \
      px1 += kFloatsPerSIMDVector;            \
      px2 += kFloatsPerSIMDVector;            \
      px3 += kFloatsPerSIMDVector;            \
      py1 += kFloatsPerSIMDVector;            \
    }                            \
    return vy;                        \
  }

  DEFINE_OP3(lerp, vecAdd(x1, (vecMul(x3, vecSub(x2, x1)))));  // lerp(a, b, mix). NB: "(x1 + (x3 * (x2 - x1)))" would be pretty but does not work on Windows
  DEFINE_OP3(clamp, vecClamp(x1, x2, x3) );          // clamp(x, minBound, maxBound)
  DEFINE_OP3(within, vecWithin(x1, x2, x3) );          // is x in the open interval [x2, x3) ?

  // ----------------------------------------------------------------
  // unary float -> int operators

  #define DEFINE_OP1_F2I(opName, opComputation)        \
  template<int VECTORS>                    \
  inline DSPVectorArrayInt<VECTORS>              \
    (opName)(const DSPVectorArray<VECTORS>& vx1)      \
  {                              \
    DSPVectorArrayInt<VECTORS> vy;              \
    const float* px1 = vx1.getConstBuffer();        \
    float* py1 = vy.getBuffer();              \
    for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)    \
    {                            \
      SIMDVectorFloat x = vecLoad(px1);          \
      vecStore((py1), (opComputation));            \
      px1 += kFloatsPerSIMDVector;            \
      py1 += kIntsPerSIMDVector;              \
    }                            \
    return vy;                        \
  }

  DEFINE_OP1_F2I(roundFloatToInt, (VecI2F(vecFloatToIntRound(x))));
  DEFINE_OP1_F2I(truncateFloatToInt, (VecI2F(vecFloatToIntTruncate(x))));

  // ----------------------------------------------------------------
  // unary int -> float operators

  #define DEFINE_OP1_I2F(opName, opComputation)        \
  template<int VECTORS>                    \
  inline DSPVectorArray<VECTORS>                \
  (opName)(const DSPVectorArrayInt<VECTORS>& vx1)      \
  {                              \
  DSPVectorArray<VECTORS> vy;              \
  const float* px1 = vx1.getConstBuffer();        \
  float* py1 = vy.getBuffer();              \
  for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)    \
  {                            \
  SIMDVectorInt x = VecF2I(vecLoad(px1));            \
  vecStore((py1), (opComputation));            \
  px1 += kIntsPerSIMDVector;              \
  py1 += kFloatsPerSIMDVector;            \
  }                            \
  return vy;                        \
  }

  DEFINE_OP1_I2F(intToFloat, (vecIntToFloat(x)));

  // ----------------------------------------------------------------
  // binary float, float -> int operators

  #define DEFINE_OP2_FF2I(opName, opComputation)        \
  template<int VECTORS>                    \
  inline DSPVectorArrayInt<VECTORS>              \
  (opName)(const DSPVectorArray<VECTORS>& vx1,      \
  const DSPVectorArray<VECTORS>& vx2)            \
  {                              \
  DSPVectorArrayInt<VECTORS> vy;              \
const float* px1 = vx1.getConstBuffer();        \
const float* px2 = vx2.getConstBuffer();        \
  float* py1 = vy.getBuffer();              \
  for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)    \
  {                            \
  SIMDVectorFloat x1 = vecLoad(px1);          \
  SIMDVectorFloat x2 = vecLoad(px2);          \
  vecStore((py1), (opComputation));            \
  px1 += kFloatsPerSIMDVector;            \
px2 += kFloatsPerSIMDVector;            \
  py1 += kIntsPerSIMDVector;              \
  }                            \
  return vy;                        \
  }

  DEFINE_OP2_FF2I(equal, (vecEqual(x1, x2)));
  DEFINE_OP2_FF2I(notEqual, (vecNotEqual(x1, x2)));
  DEFINE_OP2_FF2I(greaterThan, (vecGreaterThan(x1, x2)));
  DEFINE_OP2_FF2I(greaterThanOrEqual, (vecGreaterThanOrEqual(x1, x2)));
  DEFINE_OP2_FF2I(lessThan, (vecLessThan(x1, x2)));
  DEFINE_OP2_FF2I(lessThanOrEqual, (vecLessThanOrEqual(x1, x2)));

  // ----------------------------------------------------------------
  // ternary operators float, float int -> float

  #define DEFINE_OP3_FFI2F(opName, opComputation)          \
  template<int VECTORS>                    \
  inline DSPVectorArray<VECTORS>                \
  (opName)(const DSPVectorArray<VECTORS>& vx1,      \
  const DSPVectorArray<VECTORS>& vx2,            \
  const DSPVectorArrayInt<VECTORS>& vx3)            \
  {                              \
  DSPVectorArray<VECTORS> vy;                \
  const float* px1 = vx1.getConstBuffer();        \
  const float* px2 = vx2.getConstBuffer();        \
  const float* px3 = vx3.getConstBuffer();        \
  float* py1 = vy.getBuffer();              \
  for (int n = 0; n < kSIMDVectorsPerDSPVector*VECTORS; ++n)    \
  {                            \
  SIMDVectorFloat x1 = vecLoad(px1);          \
  SIMDVectorFloat x2 = vecLoad(px2);          \
  SIMDVectorInt x3 = VecF2I(vecLoad(px3));            \
  vecStore(py1, (opComputation));            \
  px1 += kFloatsPerSIMDVector;            \
  px2 += kFloatsPerSIMDVector;            \
  px3 += kFloatsPerSIMDVector;            \
  py1 += kFloatsPerSIMDVector;            \
  }                            \
  return vy;                        \
  }

  DEFINE_OP3_FFI2F(select, vecSelect(x1, x2, x3));          // bitwise select(resultIfTrue, resultIfFalse, conditionMask)


  // ----------------------------------------------------------------
  // single-vector index and sequence generators

  constexpr float castFn(int i) { return i; }

  inline DSPVector columnIndex()
  {
    ConstDSPVector indices(castFn);
    return indices;
  }

  // return a linear sequence from start to end, where end will fall on the first index of the
  // next vector.
  inline DSPVector rangeOpen(float start, float end)
  {
    float interval = (end - start)/(kFloatsPerDSPVector);
    return columnIndex()*DSPVector(interval) + DSPVector(start);
  }

  // return a linear sequence from start to end, where end falls on the last index of this vector.
  inline DSPVector rangeClosed(float start, float end)
  {
    float interval = (end - start)/(kFloatsPerDSPVector - 1.f);
    return columnIndex()*DSPVector(interval) + DSPVector(start);
  }

  // return a linear sequence from start to end, where start falls one sample "before"
  // this vector and end falls on the last index of this vector.
  inline DSPVector interpolateDSPVectorLinear(float start, float end)
  {
    float interval = (end - start)/(kFloatsPerDSPVector);
    return columnIndex()*DSPVector(interval) + DSPVector(start + interval);
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
  inline DSPVectorArray<COPIES*VECTORS> repeat(const DSPVectorArray<VECTORS> x1)
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
  inline DSPVectorArray<VECTORSA + VECTORSB> append(const DSPVectorArray<VECTORSA> x1, const DSPVectorArray<VECTORSB> x2)
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
  inline DSPVectorArray<VECTORSA + VECTORSB> shuffle(const DSPVectorArray<VECTORSA> x1, const DSPVectorArray<VECTORSB> x2)
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
  // low-level functional

  // Evaluate a function (void)->(float), store at each element of the DSPVectorArray and return the result.
  // x is a dummy argument just used to infer the vector size.
  template<int VECTORS>
  inline DSPVectorArray<VECTORS> map(std::function<float()> f, const DSPVectorArray<VECTORS> x)
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
  inline DSPVectorArray<VECTORS> map(std::function<float(float)> f, const DSPVectorArray<VECTORS> x)
  {
    DSPVectorArray<VECTORS> y;
    for(int n=0; n<kFloatsPerDSPVector*VECTORS; ++n)
    {
      y[n] = f(x[n]);
    }
    return y;
  }

  // Apply a function (int)->(float) to each element of the DSPVectorArrayInt x and return the result.
  template<int VECTORS>
  inline DSPVectorArray<VECTORS> map(std::function<float(float)> f, const DSPVectorArrayInt<VECTORS> x)
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
  inline DSPVectorArray<VECTORS> map(std::function<DSPVector(const DSPVector)> f, const DSPVectorArray<VECTORS> x)
  {
    DSPVectorArray<VECTORS> y;
    for(int j=0; j<VECTORS; ++j)
    {
      y.row(j) = f(x.constRow(j)); // untested
    }
    return y;
  }

  // Apply a function (DSPVector, int row)->(DSPVector) to each row of the DSPVectorArray x and return the result.
  template<int VECTORS>
  inline DSPVectorArray<VECTORS> map(std::function<DSPVector(const DSPVector, int)> f, const DSPVectorArray<VECTORS> x)
  {
    DSPVectorArray<VECTORS> y;
    for(int j=0; j<VECTORS; ++j)
    {
      y.row(j) = f(x.constRow(j), j);
    }
    return y;
  }
  // ----------------------------------------------------------------
  // for testing

  template<int VECTORS>
  inline std::ostream& operator<< (std::ostream& out, const DSPVectorArray<VECTORS>& vecArray)
  {
    out << "@" << std::hex << reinterpret_cast<unsigned long>(&vecArray) << std::dec << " ";

//    if(VECTORS > 1) out << "[   ";
    for(int v=0; v<VECTORS; ++v)
    {
    //  if(VECTORS > 1) if(v > 0) out << "\n    ";
      if(VECTORS > 1) out << "\n    v" << v << ": ";
      out << "[";
      for(int i=0; i<kFloatsPerDSPVector; ++i)
      {
        out << vecArray[v*kFloatsPerDSPVector + i] << " ";
      }
      out << "] ";
    }
//    if(VECTORS > 1) out << "]";
    return out;
  }

  template<int VECTORS>
  inline std::ostream& operator<< (std::ostream& out, const DSPVectorArrayInt<VECTORS>& vecArray)
  {
    out << "@" << std::hex << reinterpret_cast<unsigned long>(&vecArray) << std::dec << "\n ";
//    if(VECTORS > 1) out << "[   ";
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
//    if(VECTORS > 1) out << "]";
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


#endif // __ML_DSP_OPS__

