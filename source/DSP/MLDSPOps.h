// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// This module contains the DSPVectorArray / DSPVector class and basic
// operations on it. Any stateless operations on DSPVectors should also be added
// here.
//
// These objects are for building fixed DSP graphs in a functional style. The
// compiler should have many opportunities to optimize these graphs. For dynamic
// graphs changeable at runtime, see MLProcs. In general MLProcs will be written
// using DSPGens, DSPOps, DSPFilters.

#pragma once

#ifdef _WIN32
#include <memory>
#else
// #include <tr1/memory>
#endif

#ifdef __INTEL_COMPILER
#include <mathimf.h>
#endif

#if (defined(_MSC_VER) && _MSC_VER < 1900)
#define snprintf _snprintf
#endif

#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <type_traits>
#include <vector>

#include "MLDSPMath.h"
#include "MLDSPScalarMath.h"

// make_array, used in constructors

template <class Function, std::size_t... Indices>
constexpr auto make_array_helper(Function f, ml_index_sequence<Indices...>)
    -> std::array<typename std::invoke_result<Function, std::size_t>::type, sizeof...(Indices)>
{
  return {{f(Indices)...}};
}

template <int N, class Function>
constexpr auto make_array(Function f)
    -> std::array<typename std::invoke_result<Function, std::size_t>::type, N>
{
  return make_array_helper(f, ml_make_index_sequence<N>{});
}

#if (_WIN32 && (!_WIN64))
#define MANUAL_ALIGN_DSPVECTOR
#endif

// ----------------------------------------------------------------
// alignment definitions

#ifdef MANUAL_ALIGN_DSPVECTOR

const uintptr_t kDSPVectorAlignBytes = 16;
const uintptr_t kDSPVectorAlignFloats = kDSPVectorAlignBytes / sizeof(float);
const uintptr_t kDSPVectorAlignInts = kDSPVectorAlignBytes / sizeof(int);
const uintptr_t kDSPVectorBytesAlignMask = ~(kDSPVectorAlignBytes - 1);

template <type T>
inline T* DSPVectorAlignPointer(const T* p)
{
  uintptr_t pM = (uintptr_t)p;
  pM += (uintptr_t)(ml::kDSPVectorAlignBytes - 1);
  pM &= ml::kDSPVectorBytesAlignMask;
  return reinterpret_cast<T*>(pM);
}

#endif





// TODO
/*
remove MANUAL_ALIGN_DSPVECTOR
make FloatArray type
use DSPChunk<ROWS> = FLoatArray somehow
make templates that create Banks with vertical output (<COLS, ROWS> ? ) using 4x SIMD ops
*/


// ----------------------------------------------------------------
// DSPVectorArray
//
// An array of DSPVectors. Each DSPVector holds kFloatsPerDSPVector float32 samples.
// A DSPVectorArray< ROWS > holds kFloatsPerDSPVector*ROWS samples.
// Knowing the array size at compile time helps efficiency by allowing the compiler to
// unroll loops.
//
// Note that "Vector" is used in the mathematical sense of an n-tuple of real values.

namespace ml
{
template <size_t ROWS>
class DSPVectorArray
{
  // union def'n
#ifdef MANUAL_ALIGN_DSPVECTOR
  union Data
  {
    float asFloat[kFloatsPerDSPVector * ROWS + kDSPVectorAlignFloats];

    Data() {}

    Data(std::array<float, kFloatsPerDSPVector * ROWS> a)
    {
      float* py = DSPVectorAlignPointer<float>(this->asFloat);
      for (int i = 0; i < kFloatsPerDSPVector * ROWS; ++i)
      {
        py[i] = a[i];
      }
    }
  };
#else
  union Data
  {
    SIMDVectorFloat _align[kSIMDVectorsPerDSPVector * ROWS];   // unused except to force alignment
    std::array<float, kFloatsPerDSPVector * ROWS> arrayData_;  // for constexpr ctor
    float asFloat[kFloatsPerDSPVector * ROWS];

    Data() {}
    constexpr Data(std::array<float, kFloatsPerDSPVector * ROWS> a) : arrayData_(a) {}
  };

#endif

  Data data_;

 public:
  // getBuffer, getConstBuffer
#ifdef MANUAL_ALIGN_DSPVECTOR
  inline float* getBuffer() const { return DSPVectorAlignPointer<float>(data_.asFloat); }
  inline const float* getConstBuffer() const { return DSPVectorAlignPointer<float>(data_.asFloat); }
#else
  inline float* getBuffer() { return data_.asFloat; }
  inline const float* getConstBuffer() const { return data_.asFloat; }
#endif  // MANUAL_ALIGN_DSPVECTOR

  // constexpr constructor taking a std::array. Use with make_array
  constexpr DSPVectorArray(std::array<float, kFloatsPerDSPVector * ROWS> a) : data_(a) {}

  // constexpr constructor taking a function(int -> float)
  constexpr DSPVectorArray(float (*fn)(int))
      : DSPVectorArray(make_array<kFloatsPerDSPVector * ROWS>(fn))
  {
  }

  // TODO constexpr constructor taking a Projection - requires Projection
  // rewrite without std::function

  // default constructor: zeroes the data.
  // TODO this seems to be taking a lot of time! investigate
  constexpr DSPVectorArray() { data_.arrayData_.fill(0.f); }

  // conversion constructor to float.  This keeps the syntax of common DSP code
  // shorter: "va + DSPVector(1.f)" becomes just "va + 1.f".
  constexpr DSPVectorArray(float k) { operator=(k); }

  // unaligned data * ctors
  explicit DSPVectorArray(float* pData) { load(*this, pData); }
  explicit DSPVectorArray(const float* pData) { load(*this, pData); }

  // aligned data * ctors
  explicit DSPVectorArray(DSPVectorArray* pData) { loadAligned(*this, pData); }
  explicit DSPVectorArray(const DSPVectorArray* pData) { loadAligned(*this, pData); }

  inline float& operator[](size_t i) { return getBuffer()[i]; }
  inline const float operator[](size_t i) const { return getConstBuffer()[i]; }

  // = float: set each element of the DSPVectorArray to the float value k.
  inline DSPVectorArray operator=(float k)
  {
    const SIMDVectorFloat vk = vecSet1(k);
    float* py1 = getBuffer();

    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)
    {
      vecStore(py1, vk);
      py1 += kFloatsPerSIMDVector;
    }
    return *this;
  }

  // default copy and = constructors.

  DSPVectorArray(const DSPVectorArray& x1) noexcept = default;
  DSPVectorArray& operator=(const DSPVectorArray& x1) noexcept = default;

  // equality by value
  bool operator==(const DSPVectorArray& x1) const
  {
    const float* px1 = x1.getConstBuffer();
    const float* py1 = getConstBuffer();

    for (int n = 0; n < kFloatsPerDSPVector * ROWS; ++n)
    {
      if (py1[n] != px1[n]) return false;
    }
    return true;
  }

  // TODO compare the performance of getting rid of everything between here and row(), and
  // just using row() and constRow() for reading and writing

  // return row J from this DSPVectorArray, when J is known at compile time.
  template <int J>
  inline DSPVectorArray<1> getRowVector() const
  {
    static_assert((J >= 0) && (J < ROWS), "getRowVector index out of bounds");
    return getRowVectorUnchecked(J);
  }

  // set row J of this DSPVectorArray to x1, when J is known at compile time.
  template <int J>
  inline void setRowVector(const DSPVectorArray<1> x1)
  {
    static_assert((J >= 0) && (J < ROWS), "setRowVector index out of bounds");
    setRowVectorUnchecked(J, x1);
  }

#ifdef MANUAL_ALIGN_DSPVECTOR

  // get a row vector j when j is not known at compile time.
  inline DSPVectorArray<1> getRowVectorUnchecked(size_t j) const
  {
    DSPVectorArray<1> vy;
    const float* px1 = getConstBuffer() + kFloatsPerDSPVector * j;
    float* py1 = vy.getBuffer();

    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      py1[n] = px1[n];
    }
    return vy;
  }

  // set a row vector j when j is not known at compile time.
  inline void setRowVectorUnchecked(size_t j, const DSPVectorArray<1> x1)
  {
    const float* px1 = x1.getConstBuffer();
    float* py1 = getBuffer() + kFloatsPerDSPVector * j;

    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      py1[n] = px1[n];
    }
  }
#else

  // get a row vector j when j is not known at compile time.
  inline DSPVectorArray<1> getRowVectorUnchecked(size_t j) const
  {
    DSPVectorArray<1> vy;
    const float* px1 = getConstBuffer() + kFloatsPerDSPVector * j;
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
  inline void setRowVectorUnchecked(size_t j, const DSPVectorArray<1> x1)
  {
    const float* px1 = x1.getConstBuffer();
    float* py1 = getBuffer() + kFloatsPerDSPVector * j;

    for (int n = 0; n < kSIMDVectorsPerDSPVector; ++n)
    {
      vecStore(py1, vecLoad(px1));
      px1 += kFloatsPerSIMDVector;
      py1 += kFloatsPerSIMDVector;
    }
  }
#endif

  // return a const pointer to the first element in row J of this
  // DSPVectorArray.
  inline const float* getRowDataConst(int j) const
  {
    const float* py1 = getConstBuffer() + kFloatsPerDSPVector * j;
    return py1;
  }

  // return a pointer to the first element in row J of this DSPVectorArray.
  inline float* getRowData(int j)
  {
    float* py1 = getBuffer() + kFloatsPerDSPVector * j;
    return py1;
  }

  // return a reference to a row of this DSPVectorArray.
  inline DSPVectorArray<1>& row(int j)
  {
    float* py1 = getBuffer() + kFloatsPerDSPVector * j;
    DSPVectorArray<1>* pRow = reinterpret_cast<DSPVectorArray<1>*>(py1);
    return *pRow;
  }

  // return a const reference to a row of this DSPVectorArray.
  inline const DSPVectorArray<1>& constRow(int j) const
  {
    const float* py1 = getConstBuffer() + kFloatsPerDSPVector * j;
    const DSPVectorArray<1>* pRow = reinterpret_cast<const DSPVectorArray<1>*>(py1);
    return *pRow;
  }

  inline DSPVectorArray& operator+=(const DSPVectorArray& x1)
  {
    *this = add(*this, x1);
    return *this;
  }
  inline DSPVectorArray& operator-=(const DSPVectorArray& x1)
  {
    *this = subtract(*this, x1);
    return *this;
  }
  inline DSPVectorArray& operator*=(const DSPVectorArray& x1)
  {
    *this = multiply(*this, x1);
    return *this;
  }
  inline DSPVectorArray& operator/=(const DSPVectorArray& x1)
  {
    *this = divide(*this, x1);
    return *this;
  }

  // binary operators - defining these here inside the class as non-template
  // functions enables the compiler to call implicit conversions on either
  // argument.

  friend inline DSPVectorArray operator+(const DSPVectorArray& x1, const DSPVectorArray& x2)
  {
    return add(x1, x2);
  }
  friend inline DSPVectorArray operator-(const DSPVectorArray& x1)
  {
    // TODO should be able to declare this constexpr!
    DSPVectorArray zero(0.f);
    
    return subtract(zero, x1);
  }
  friend inline DSPVectorArray operator-(const DSPVectorArray& x1, const DSPVectorArray& x2)
  {
    return subtract(x1, x2);
  }
  friend inline DSPVectorArray operator*(const DSPVectorArray& x1, const DSPVectorArray& x2)
  {
    return multiply(x1, x2);
  }
  friend inline DSPVectorArray operator/(const DSPVectorArray& x1, const DSPVectorArray& x2)
  {
    return divide(x1, x2);
  }
};  // class DSPVectorArray

// ----------------------------------------------------------------
// DSPVector
//
// Special case of the DSPVectorArray with one row.
// The most common data type in a typical DSP function.

typedef DSPVectorArray<1> DSPVector;

// ----------------------------------------------------------------
// DSPVectorArrayInt
//
// DSP Vector holding int32 values.

constexpr size_t kIntsPerDSPVector = kFloatsPerDSPVector;

template <size_t ROWS>
class DSPVectorArrayInt
{
#ifdef MANUAL_ALIGN_DSPVECTOR
  union Data
  {
    std::array<int, kIntsPerDSPVector * ROWS + kDSPVectorAlignInts> mArrayData;
    int32_t asInt[kIntsPerDSPVector * ROWS + kDSPVectorAlignInts];
    float asFloat[kFloatsPerDSPVector * ROWS + kDSPVectorAlignFloats];

    Data() {}

    Data(std::array<int, kIntsPerDSPVector * ROWS> a)
    {
      int* py = DSPVectorAlignPointer<int>(this->asFloat);
      for (int i = 0; i < kIntsPerDSPVector * ROWS; ++i)
      {
        py[i] = a[i];
      }
    }
  };
#else
  union Data
  {
    SIMDVectorInt _align[kSIMDVectorsPerDSPVector * ROWS];     // unused except to force alignment
    std::array<int32_t, kIntsPerDSPVector * ROWS> arrayData_;  // for constexpr ctor
    int32_t asInt[kIntsPerDSPVector * ROWS];
    float asFloat[kFloatsPerDSPVector * ROWS];

    Data() {}
    constexpr Data(std::array<int32_t, kIntsPerDSPVector * ROWS> a) : arrayData_(a) {}
  };
#endif  // MANUAL_ALIGN_DSPVECTOR

 public:
  Data data_;

  // getBuffer, getConstBuffer
#ifdef MANUAL_ALIGN_DSPVECTOR
  inline float* getBuffer() const { return DSPVectorAlignPointer<float>(data_.asFloat); }
  inline const float* getConstBuffer() const { return DSPVectorAlignPointer<float>(data_.asFloat); }
  inline int32_t* getBufferInt() const { return DSPVectorAlignPointer<float>(data_.asInt); }
  inline const int32_t* getConstBufferInt() const
  {
    return DSPVectorAlignPointer<float>(data_.asInt);
  }
#else
  inline float* getBuffer() { return data_.asFloat; }
  inline const float* getConstBuffer() const { return data_.asFloat; }
  inline int32_t* getBufferInt() { return data_.asInt; }
  inline const int32_t* getConstBufferInt() const { return data_.asInt; }
#endif  // MANUAL_ALIGN_DSPVECTOR

  explicit DSPVectorArrayInt() { operator=(0); }
  explicit DSPVectorArrayInt(int32_t k) { operator=(k); }

  inline int32_t& operator[](int i) { return getBufferInt()[i]; }
  inline const int32_t operator[](int i) const { return getConstBufferInt()[i]; }

  // set each element of the DSPVectorArray to the int32_t value k.
  inline DSPVectorArrayInt operator=(int32_t k)
  {
    SIMDVectorFloat vk = VecI2F(vecSetInt1(k));
    int32_t* py1 = getBufferInt();

    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)
    {
      vecStore(reinterpret_cast<float*>(py1), vk);
      py1 += kIntsPerSIMDVector;
    }
    return *this;
  }

  // constexpr constructor taking a std::array. Use with make_array
  constexpr DSPVectorArrayInt(std::array<int32_t, kFloatsPerDSPVector * ROWS> a) : data_(a) {}

  // constexpr constructor taking a function(int -> int)
  constexpr DSPVectorArrayInt(int (*fn)(int))
      : DSPVectorArrayInt(make_array<kFloatsPerDSPVector * ROWS>(fn))
  {
  }

  DSPVectorArrayInt(const DSPVectorArrayInt& x1) noexcept = default;
  DSPVectorArrayInt& operator=(const DSPVectorArrayInt& x1) noexcept = default;

  // equality by value
  bool operator==(const DSPVectorArrayInt& x1)
  {
    const int* px1 = x1.getConstBufferInt();
    const int* py1 = getConstBufferInt();

    for (int n = 0; n < kIntsPerDSPVector * ROWS; ++n)
    {
      if (py1[n] != px1[n]) return false;
    }
    return true;
  }

  // return a reference to a row of this DSPVectorArrayInt.
  inline DSPVectorArrayInt<1>& row(int j)
  {
    float* py1 = getBuffer() + kIntsPerDSPVector * j;
    DSPVectorArrayInt<1>* pRow = reinterpret_cast<DSPVectorArrayInt<1>*>(py1);
    return *pRow;
  }

  // return a reference to a row of this DSPVectorArrayInt.
  inline const DSPVectorArrayInt<1>& constRow(int j) const
  {
    const float* py1 = getConstBuffer() + kIntsPerDSPVector * j;
    const DSPVectorArrayInt<1>* pRow = reinterpret_cast<const DSPVectorArrayInt<1>*>(py1);
    return *pRow;
  }

  friend inline DSPVectorArrayInt operator+(const DSPVectorArrayInt& x1,
                                            const DSPVectorArrayInt& x2)
  {
    return addInt32(x1, x2);
  }

  friend inline DSPVectorArrayInt operator-(const DSPVectorArrayInt& x1)
  {
    DSPVectorArrayInt zero(0);
    return subtractInt32(zero, x1);
  }
  
  friend inline DSPVectorArrayInt operator-(const DSPVectorArrayInt& x1,
                                            const DSPVectorArrayInt& x2)
  {
    return subtractInt32(x1, x2);
  }
  
};  // class DSPVectorArrayInt

typedef DSPVectorArrayInt<1> DSPVectorInt;

// ----------------------------------------------------------------
// DSPVectorDynamic: for holding a number of DSPVectors only known at runtime.

class DSPVectorDynamic final
{
 public:
  DSPVectorDynamic() = default;
  ~DSPVectorDynamic() = default;

  explicit DSPVectorDynamic(size_t rows) { data_.resize(rows); }
  void resize(size_t rows) { data_.resize(rows); }
  size_t size() const { return data_.size(); }
  DSPVector& operator[](int j) { return data_[j]; }
  const DSPVector& operator[](int j) const { return data_[j]; }

 private:
  std::vector<DSPVector> data_;
};

// ----------------------------------------------------------------
// load and store

// some loads and stores may be unaligned, let std::copy handle this
template <size_t ROWS>
inline void load(DSPVectorArray<ROWS>& vecDest, const float* pSrc)
{
  std::copy(pSrc, pSrc + kFloatsPerDSPVector * ROWS, vecDest.getBuffer());
}

template <size_t ROWS>
inline void store(const DSPVectorArray<ROWS>& vecSrc, float* pDest)
{
  std::copy(vecSrc.getConstBuffer(), vecSrc.getConstBuffer() + kFloatsPerDSPVector * ROWS, pDest);
}

// if the pointers are known to be aligned, copy as SIMD vectors
template <size_t ROWS>
inline void loadAligned(DSPVectorArray<ROWS>& vecDest, const float* pSrc)
{
  const float* px1 = pSrc;
  float* py1 = vecDest.getBuffer();

  for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)
  {
    vecStore(py1, vecLoad(px1));
    px1 += kFloatsPerSIMDVector;
    py1 += kFloatsPerSIMDVector;
  }
}

template <size_t ROWS>
inline void storeAligned(const DSPVectorArray<ROWS>& vecSrc, float* pDest)
{
  const float* px1 = vecSrc.getConstBuffer();
  float* py1 = pDest;

  for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)
  {
    vecStore(py1, vecLoad(px1));
    px1 += kFloatsPerSIMDVector;
    py1 += kFloatsPerSIMDVector;
  }
}

// ----------------------------------------------------------------
// unary vector operators (float) -> float

#define DEFINE_OP1(opName, opComputation)                              \
  template <size_t ROWS>                                               \
  inline DSPVectorArray<ROWS>(opName)(const DSPVectorArray<ROWS>& vx1) \
  {                                                                    \
    DSPVectorArray<ROWS> vy;                                           \
    const float* px1 = vx1.getConstBuffer();                           \
    float* py1 = vy.getBuffer();                                       \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)          \
    {                                                                  \
      SIMDVectorFloat x = vecLoad(px1);                                \
      vecStore(py1, (opComputation));                                  \
      px1 += kFloatsPerSIMDVector;                                     \
      py1 += kFloatsPerSIMDVector;                                     \
    }                                                                  \
    return vy;                                                         \
  }

DEFINE_OP1(recipApprox, (vecRecipApprox(x)));
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

// cubic tanh approx
DEFINE_OP1(tanhApprox, (vecTanhApprox(x)));

// ----------------------------------------------------------------
// binary vector operators (float, float) -> float

#define DEFINE_OP2(opName, opComputation)                              \
  template <size_t ROWS>                                               \
  inline DSPVectorArray<ROWS>(opName)(const DSPVectorArray<ROWS>& vx1, \
                                      const DSPVectorArray<ROWS>& vx2) \
  {                                                                    \
    DSPVectorArray<ROWS> vy;                                           \
    const float* px1 = vx1.getConstBuffer();                           \
    const float* px2 = vx2.getConstBuffer();                           \
    float* py1 = vy.getBuffer();                                       \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)          \
    {                                                                  \
      SIMDVectorFloat x1 = vecLoad(px1);                               \
      SIMDVectorFloat x2 = vecLoad(px2);                               \
      vecStore(py1, (opComputation));                                  \
      px1 += kFloatsPerSIMDVector;                                     \
      px2 += kFloatsPerSIMDVector;                                     \
      py1 += kFloatsPerSIMDVector;                                     \
    }                                                                  \
    return vy;                                                         \
  }

DEFINE_OP2(add, (vecAdd(x1, x2)));
DEFINE_OP2(subtract, (vecSub(x1, x2)));
DEFINE_OP2(multiply, (vecMul(x1, x2)));
DEFINE_OP2(divide, (vecDiv(x1, x2)));

DEFINE_OP2(divideApprox, vecDivApprox(x1, x2));
DEFINE_OP2(pow, (vecExp(vecMul(vecLog(x1), x2))));
DEFINE_OP2(powApprox, (vecExpApprox(vecMul(vecLogApprox(x1), x2))));
DEFINE_OP2(min, (vecMin(x1, x2)));
DEFINE_OP2(max, (vecMax(x1, x2)));

// ----------------------------------------------------------------
// binary vector operators (float, float) -> float
// from multiple-row and single-row operands

#define DEFINE_OP2_MS(opName, opComputation)                           \
  template <size_t ROWS>                                               \
  inline DSPVectorArray<ROWS>(opName)(const DSPVectorArray<ROWS>& vx1, \
                                      const DSPVectorArray<1>& vx2)    \
  {                                                                    \
    DSPVectorArray<ROWS> vy;                                           \
    const float* px1 = vx1.getConstBuffer();                           \
    const float* px2 = vx2.getConstBuffer();                           \
    float* py1 = vy.getBuffer();                                       \
    size_t px2Offset = 0;                                              \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)          \
    {                                                                  \
      SIMDVectorFloat x1 = vecLoad(px1);                               \
      SIMDVectorFloat x2 = vecLoad(px2 + px2Offset);                   \
      vecStore(py1, (opComputation));                                  \
      px1 += kFloatsPerSIMDVector;                                     \
      px2Offset += kFloatsPerSIMDVector;                               \
      px2Offset &= kFloatsPerDSPVector - 1;                            \
      py1 += kFloatsPerSIMDVector;                                     \
    }                                                                  \
    return vy;                                                         \
  }

DEFINE_OP2_MS(add1, (vecAdd(x1, x2)));
DEFINE_OP2_MS(subtract1, (vecSub(x1, x2)));
DEFINE_OP2_MS(multiply1, (vecMul(x1, x2)));
DEFINE_OP2_MS(divide1, (vecDiv(x1, x2)));

DEFINE_OP2_MS(divideApprox1, vecDivApprox(x1, x2));
DEFINE_OP2_MS(pow1, (vecExp(vecMul(vecLog(x1), x2))));
DEFINE_OP2_MS(powApprox1, (vecExpApprox(vecMul(vecLogApprox(x1), x2))));
DEFINE_OP2_MS(min1, (vecMin(x1, x2)));
DEFINE_OP2_MS(max1, (vecMax(x1, x2)));

// ----------------------------------------------------------------
// binary vector operators (int32, int32) -> int32

#define DEFINE_OP2_INT32(opName, opComputation)                              \
  template <size_t ROWS>                                                     \
  inline DSPVectorArrayInt<ROWS>(opName)(const DSPVectorArrayInt<ROWS>& vx1, \
                                         const DSPVectorArrayInt<ROWS>& vx2) \
  {                                                                          \
    DSPVectorArrayInt<ROWS> vy;                                              \
    const float* px1 = vx1.getConstBuffer();                                 \
    const float* px2 = vx2.getConstBuffer();                                 \
    float* py1 = vy.getBuffer();                                             \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)                \
    {                                                                        \
      SIMDVectorInt x1 = VecF2I(vecLoad(px1));                               \
      SIMDVectorInt x2 = VecF2I(vecLoad(px2));                               \
      vecStore(py1, VecI2F(opComputation));                                  \
      px1 += kIntsPerSIMDVector;                                             \
      px2 += kIntsPerSIMDVector;                                             \
      py1 += kIntsPerSIMDVector;                                             \
    }                                                                        \
    return vy;                                                               \
  }

DEFINE_OP2_INT32(subtractInt32, (vecSubInt(x1, x2)));
DEFINE_OP2_INT32(addInt32, (vecAddInt(x1, x2)));

// ----------------------------------------------------------------
// ternary vector operators (float, float, float) -> float

#define DEFINE_OP3(opName, opComputation)                              \
  template <size_t ROWS>                                               \
  inline DSPVectorArray<ROWS>(opName)(const DSPVectorArray<ROWS>& vx1, \
                                      const DSPVectorArray<ROWS>& vx2, \
                                      const DSPVectorArray<ROWS>& vx3) \
  {                                                                    \
    DSPVectorArray<ROWS> vy;                                           \
    const float* px1 = vx1.getConstBuffer();                           \
    const float* px2 = vx2.getConstBuffer();                           \
    const float* px3 = vx3.getConstBuffer();                           \
    float* py1 = vy.getBuffer();                                       \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)          \
    {                                                                  \
      SIMDVectorFloat x1 = vecLoad(px1);                               \
      SIMDVectorFloat x2 = vecLoad(px2);                               \
      SIMDVectorFloat x3 = vecLoad(px3);                               \
      vecStore(py1, (opComputation));                                  \
      px1 += kFloatsPerSIMDVector;                                     \
      px2 += kFloatsPerSIMDVector;                                     \
      px3 += kFloatsPerSIMDVector;                                     \
      py1 += kFloatsPerSIMDVector;                                     \
    }                                                                  \
    return vy;                                                         \
  }

DEFINE_OP3(lerp, vecAdd(x1, (vecMul(x3, vecSub(x2, x1)))));       // x = lerp(a, b, mix)
DEFINE_OP3(inverseLerp, vecDiv(vecSub(x3, x1), vecSub(x2, x1)));  // mix = inverseLerp(a, b, x)

DEFINE_OP3(clamp, vecClamp(x1, x2, x3));    // clamp(x, minBound, maxBound)
DEFINE_OP3(within, vecWithin(x1, x2, x3));  // is x in the open interval [x2, x3) ?

// ----------------------------------------------------------------
// lerp two vectors with scalar float mixture (constant over each vector)

template <size_t ROWS>
inline DSPVectorArray<ROWS> lerp(const DSPVectorArray<ROWS>& vx1, const DSPVectorArray<ROWS>& vx2,
                                 float m)
{
  DSPVectorArray<ROWS> vy;
  const float* px1 = vx1.getConstBuffer();
  const float* px2 = vx2.getConstBuffer();
  DSPVector vmix(m);
  float* py1 = vy.getBuffer();
  const SIMDVectorFloat vConstMix = vecSet1(m);

  for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)
  {
    SIMDVectorFloat x1 = vecLoad(px1);
    SIMDVectorFloat x2 = vecLoad(px2);
    vecStore(py1, vecAdd(x1, (vecMul(vConstMix, vecSub(x2, x1)))));
    px1 += kFloatsPerSIMDVector;
    px2 += kFloatsPerSIMDVector;
    py1 += kFloatsPerSIMDVector;
  }
  return vy;
}

// ----------------------------------------------------------------
// vector operators (float) -> int

#define DEFINE_OP1_F2I(opName, opComputation)                             \
  template <size_t ROWS>                                                  \
  inline DSPVectorArrayInt<ROWS>(opName)(const DSPVectorArray<ROWS>& vx1) \
  {                                                                       \
    DSPVectorArrayInt<ROWS> vy;                                           \
    const float* px1 = vx1.getConstBuffer();                              \
    float* py1 = vy.getBuffer();                                          \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)             \
    {                                                                     \
      SIMDVectorFloat x = vecLoad(px1);                                   \
      vecStore((py1), (opComputation));                                   \
      px1 += kFloatsPerSIMDVector;                                        \
      py1 += kIntsPerSIMDVector;                                          \
    }                                                                     \
    return vy;                                                            \
  }

DEFINE_OP1_F2I(roundFloatToInt, (VecI2F(vecFloatToIntRound(x))));
DEFINE_OP1_F2I(truncateFloatToInt, (VecI2F(vecFloatToIntTruncate(x))));

// ----------------------------------------------------------------
// vector operators (int) -> float

#define DEFINE_OP1_I2F(opName, opComputation)                             \
  template <size_t ROWS>                                                  \
  inline DSPVectorArray<ROWS>(opName)(const DSPVectorArrayInt<ROWS>& vx1) \
  {                                                                       \
    DSPVectorArray<ROWS> vy;                                              \
    const float* px1 = vx1.getConstBuffer();                              \
    float* py1 = vy.getBuffer();                                          \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)             \
    {                                                                     \
      SIMDVectorInt x = VecF2I(vecLoad(px1));                             \
      vecStore((py1), (opComputation));                                   \
      px1 += kIntsPerSIMDVector;                                          \
      py1 += kFloatsPerSIMDVector;                                        \
    }                                                                     \
    return vy;                                                            \
  }

DEFINE_OP1_I2F(intToFloat, (vecIntToFloat(x)));
DEFINE_OP1_I2F(unsignedIntToFloat, (vecUnsignedIntToFloat(x)));

// ----------------------------------------------------------------
// using the conversions above, define fractionalPart

DEFINE_OP1(fractionalPart, (vecSub(x, vecIntToFloat(vecFloatToIntTruncate(x)))));

// ----------------------------------------------------------------
// binary float vector, float vector -> int vector operators

#define DEFINE_OP2_FF2I(opName, opComputation)                            \
  template <size_t ROWS>                                                  \
  inline DSPVectorArrayInt<ROWS>(opName)(const DSPVectorArray<ROWS>& vx1, \
                                         const DSPVectorArray<ROWS>& vx2) \
  {                                                                       \
    DSPVectorArrayInt<ROWS> vy;                                           \
    const float* px1 = vx1.getConstBuffer();                              \
    const float* px2 = vx2.getConstBuffer();                              \
    float* py1 = vy.getBuffer();                                          \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)             \
    {                                                                     \
      SIMDVectorFloat x1 = vecLoad(px1);                                  \
      SIMDVectorFloat x2 = vecLoad(px2);                                  \
      vecStore((py1), (opComputation));                                   \
      px1 += kFloatsPerSIMDVector;                                        \
      px2 += kFloatsPerSIMDVector;                                        \
      py1 += kIntsPerSIMDVector;                                          \
    }                                                                     \
    return vy;                                                            \
  }

DEFINE_OP2_FF2I(equal, (vecEqual(x1, x2)));
DEFINE_OP2_FF2I(notEqual, (vecNotEqual(x1, x2)));
DEFINE_OP2_FF2I(greaterThan, (vecGreaterThan(x1, x2)));
DEFINE_OP2_FF2I(greaterThanOrEqual, (vecGreaterThanOrEqual(x1, x2)));
DEFINE_OP2_FF2I(lessThan, (vecLessThan(x1, x2)));
DEFINE_OP2_FF2I(lessThanOrEqual, (vecLessThanOrEqual(x1, x2)));

// ----------------------------------------------------------------
// ternary operators float vector, float vector, int vector -> float vector

#define DEFINE_OP3_FFI2F(opName, opComputation)                           \
  template <size_t ROWS>                                                  \
  inline DSPVectorArray<ROWS>(opName)(const DSPVectorArray<ROWS>& vx1,    \
                                      const DSPVectorArray<ROWS>& vx2,    \
                                      const DSPVectorArrayInt<ROWS>& vx3) \
  {                                                                       \
    DSPVectorArray<ROWS> vy;                                              \
    const float* px1 = vx1.getConstBuffer();                              \
    const float* px2 = vx2.getConstBuffer();                              \
    const float* px3 = vx3.getConstBuffer();                              \
    float* py1 = vy.getBuffer();                                          \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)             \
    {                                                                     \
      SIMDVectorFloat x1 = vecLoad(px1);                                  \
      SIMDVectorFloat x2 = vecLoad(px2);                                  \
      SIMDVectorInt x3 = VecF2I(vecLoad(px3));                            \
      vecStore(py1, (opComputation));                                     \
      px1 += kFloatsPerSIMDVector;                                        \
      px2 += kFloatsPerSIMDVector;                                        \
      px3 += kFloatsPerSIMDVector;                                        \
      py1 += kFloatsPerSIMDVector;                                        \
    }                                                                     \
    return vy;                                                            \
  }

DEFINE_OP3_FFI2F(select, vecSelect(x1, x2, x3));  // bitwise select(resultIfTrue,
                                                  // resultIfFalse, conditionMask)

// ----------------------------------------------------------------
// ternary operators int vector, int vector, int vector -> int vector

#define DEFINE_OP3_III2I(opName, opComputation)                              \
  template <size_t ROWS>                                                     \
  inline DSPVectorArrayInt<ROWS>(opName)(const DSPVectorArrayInt<ROWS>& vx1, \
                                         const DSPVectorArrayInt<ROWS>& vx2, \
                                         const DSPVectorArrayInt<ROWS>& vx3) \
  {                                                                          \
    DSPVectorArrayInt<ROWS> vy;                                              \
    const float* px1 = vx1.getConstBuffer();                                 \
    const float* px2 = vx2.getConstBuffer();                                 \
    const float* px3 = vx3.getConstBuffer();                                 \
    float* py1 = vy.getBuffer();                                             \
    for (int n = 0; n < kSIMDVectorsPerDSPVector * ROWS; ++n)                \
    {                                                                        \
      SIMDVectorInt x1 = VecF2I(vecLoad(px1));                               \
      SIMDVectorInt x2 = VecF2I(vecLoad(px2));                               \
      SIMDVectorInt x3 = VecF2I(vecLoad(px3));                               \
      vecStore(py1, VecI2F(opComputation));                                  \
      px1 += kIntsPerSIMDVector;                                             \
      px2 += kIntsPerSIMDVector;                                             \
      px3 += kIntsPerSIMDVector;                                             \
      py1 += kIntsPerSIMDVector;                                             \
    }                                                                        \
    return vy;                                                               \
  }

DEFINE_OP3_III2I(select, vecSelect(x1, x2, x3));  // bitwise select(resultIfTrue,
                                                  // resultIfFalse, conditionMask)

// ----------------------------------------------------------------
// n-ary operators

// add (a, b, c, ...)

template <size_t ROWS>
DSPVectorArray<ROWS> add(DSPVectorArray<ROWS> a)
{
  return a;
}

template <size_t ROWS, typename... Args>
DSPVectorArray<ROWS> add(DSPVectorArray<ROWS> first, Args... args)
{
  // the + here is the operator defined using vecAdd() above
  return first + add(args...);
}

// ----------------------------------------------------------------
// constexpr definitions

#ifdef MANUAL_ALIGN_DSPVECTOR

// it seems unlikely that the constexpr ctors can be made to compile with manual
// alignment, so we give up on constexpr for 32-bit Windows.
#define ConstDSPVector static DSPVector
#define ConstDSPVectorArray static DSPVectorArray
#define ConstDSPVectorInt static DSPVectorInt
#define ConstDSPVectorArrayInt static DSPVectorArrayInt

#else

#define ConstDSPVector constexpr DSPVector
#define ConstDSPVectorArray constexpr DSPVectorArray
#define ConstDSPVectorInt constexpr DSPVectorInt
#define ConstDSPVectorArrayInt constexpr DSPVectorArrayInt

#endif

// ----------------------------------------------------------------
// single-vector index and sequence generators

constexpr float intToFloatCastFn(int i) { return (float)i; }
constexpr int indexFn(int i) { return i; }

inline ConstDSPVector columnIndex() { return (make_array<kFloatsPerDSPVector>(intToFloatCastFn)); }
inline ConstDSPVectorInt columnIndexInt() { return (make_array<kIntsPerDSPVector>(indexFn)); }

// return a linear sequence from start to end, where end will fall on the first
// index of the next vector.
inline DSPVector rangeOpen(float start, float end)
{
  float interval = (end - start) / (kFloatsPerDSPVector);
  return columnIndex() * DSPVector(interval) + DSPVector(start);
}

// return a linear sequence from start to end, where end falls on the last index
// of this vector.
inline DSPVector rangeClosed(float start, float end)
{
  float interval = (end - start) / (kFloatsPerDSPVector - 1.f);
  return columnIndex() * DSPVector(interval) + DSPVector(start);
}

// return a linear sequence from start to end, where start falls one sample
// "before" this vector and end falls on the last index of this vector.
inline DSPVector interpolateDSPVectorLinear(float start, float end)
{
  float interval = (end - start) / (kFloatsPerDSPVector);
  return columnIndex() * DSPVector(interval) + DSPVector(start + interval);
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
  constexpr float kGain = 1.0f / kFloatsPerDSPVector;
  return sum(x) * kGain;
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
// normalize

template <size_t ROWS>
inline DSPVectorArray<ROWS> normalize(const DSPVectorArray<ROWS>& x1)
{
  DSPVectorArray<ROWS> vy;
  for (int j = 0; j < ROWS; ++j)
  {
    auto inputRow = x1.getRowVectorUnchecked(j);
    vy.setRowVectorUnchecked(j, inputRow / sum(inputRow));
  }
  return vy;
}

// ----------------------------------------------------------------
// row-wise operations and conversions

// for the given output ROWS and given an input DSPVectorArray with N rows,
// repeat all the input rows enough times to fill the output DSPVectorArray.
template <size_t ROWS, size_t N>
inline DSPVectorArray<ROWS * N> repeatRows(const DSPVectorArray<N>& x1)
{
  DSPVectorArray<ROWS * N> vy;
  for (int j = 0, k = 0; j < ROWS * N; ++j)
  {
    vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(k));
    if (++k >= N) k = 0;
  }
  return vy;
}

// for the given ROWS and given an input DSPVectorArray x with N rows,
// stretch x by repeating rows as necessary to make an output DSPVectorArray
// with ROWS rows.
template <size_t ROWS, size_t N>
inline DSPVectorArray<ROWS> stretchRows(const DSPVectorArray<N>& x)
{
  DSPVectorArray<ROWS> vy;
  for (int j = 0; j < ROWS; ++j)
  {
    int k = roundf((j * (N - 1.f)) / (ROWS - 1.f));
    vy.setRowVectorUnchecked(j, x.getRowVectorUnchecked(k));
  }
  return vy;
}

// for the given ROWS and given an input DSPVectorArray x with N rows,
// fill an output array by copying rows of the input, then adding rows of zeros as
// necessary.
template <size_t ROWS, size_t N>
inline DSPVectorArray<ROWS> zeroPadRows(const DSPVectorArray<N>& x)
{
  // default constructor currently zero-fills
  DSPVectorArray<ROWS> vy;
  constexpr size_t rowsToCopy = min(ROWS, N);
  for (int j = 0; j < rowsToCopy; ++j)
  {
    vy.setRowVectorUnchecked(j, x.getRowVectorUnchecked(j));
  }
  return vy;
}

// Shift the array down by the number of rows given in rowsToShift.
// Any rows shifted in from outside the range [0, ROWS) are zeroed. Negative
// shifts are OK.
template <size_t ROWS>
inline DSPVectorArray<ROWS> shiftRows(const DSPVectorArray<ROWS>& x, int rowsToShift)
{
  DSPVectorArray<ROWS> vy;
  int k = -rowsToShift;
  for (int j = 0; j < ROWS; ++j)
  {
    if (within(k, 0, static_cast<int>(ROWS)))
    {
      vy.setRowVectorUnchecked(j, x.getRowVectorUnchecked(k));
    }
    else
    {
      vy.setRowVectorUnchecked(j, 0.f);
    }
    ++k;
  }
  return vy;
}

// Rotate the array down by the number of rows given in rowsToRotate.
// Any rows rotated in from outside the range [0, ROWS) are wrapped. Negative
// rotations are OK.
template <size_t ROWS>
inline DSPVectorArray<ROWS> rotateRows(const DSPVectorArray<ROWS>& x, int rowsToRotate)
{
  DSPVectorArray<ROWS> vy;

  // get start index k to which row 0 is mapped
  int k = modulo(-rowsToRotate, ROWS);
  for (int j = 0; j < ROWS; ++j)
  {
    vy.setRowVectorUnchecked(j, x.getRowVectorUnchecked(k));
    if (++k >= ROWS) k = 0;
  }
  return vy;
}

// ----------------------------------------------------------------
// row-wise combining

// concatRows with two arguments: append one DSPVectorArray after another.
template <size_t ROWSA, size_t ROWSB>
inline DSPVectorArray<ROWSA + ROWSB> concatRows(const DSPVectorArray<ROWSA>& x1,
                                                const DSPVectorArray<ROWSB>& x2)
{
  DSPVectorArray<ROWSA + ROWSB> vy;
  for (int j = 0; j < ROWSA; ++j)
  {
    vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(j));
  }
  for (int j = 0; j < ROWSB; ++j)
  {
    vy.setRowVectorUnchecked(j + ROWSA, x2.getRowVectorUnchecked(j));
  }
  return vy;
}

// concatRows with three arguments.
template <size_t ROWSA, size_t ROWSB, size_t ROWSC>
inline DSPVectorArray<ROWSA + ROWSB + ROWSC> concatRows(const DSPVectorArray<ROWSA>& x1,
                                                        const DSPVectorArray<ROWSB>& x2,
                                                        const DSPVectorArray<ROWSC>& x3)
{
  DSPVectorArray<ROWSA + ROWSB + ROWSC> vy;
  for (int j = 0; j < ROWSA; ++j)
  {
    vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(j));
  }
  for (int j = 0; j < ROWSB; ++j)
  {
    vy.setRowVectorUnchecked(j + ROWSA, x2.getRowVectorUnchecked(j));
  }
  for (int j = 0; j < ROWSC; ++j)
  {
    vy.setRowVectorUnchecked(j + ROWSA + ROWSB, x3.getRowVectorUnchecked(j));
  }
  return vy;
}

// NOTE: of course all this repeated code looks bad, but
// i'm not sure there is a reasonable way to generalize this in C++11. The problem is determining
// the number of rows in the output type. It should be possible with return type deduction (auto) in
// C++14.
// TODO variadic templates can expand into a template argument list, so maybe this IS possible
// by wrapping the concatRows template in another template - give it a try.

// concatRows with four arguments.
template <size_t ROWSA, size_t ROWSB, size_t ROWSC, size_t ROWSD>
inline DSPVectorArray<ROWSA + ROWSB + ROWSC + ROWSD> concatRows(const DSPVectorArray<ROWSA>& x1,
                                                                const DSPVectorArray<ROWSB>& x2,
                                                                const DSPVectorArray<ROWSC>& x3,
                                                                const DSPVectorArray<ROWSD>& x4)
{
  DSPVectorArray<ROWSA + ROWSB + ROWSC + ROWSD> vy;
  for (int j = 0; j < ROWSA; ++j)
  {
    vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(j));
  }
  for (int j = 0; j < ROWSB; ++j)
  {
    vy.setRowVectorUnchecked(j + ROWSA, x2.getRowVectorUnchecked(j));
  }
  for (int j = 0; j < ROWSC; ++j)
  {
    vy.setRowVectorUnchecked(j + ROWSA + ROWSB, x3.getRowVectorUnchecked(j));
  }
  for (int j = 0; j < ROWSD; ++j)
  {
    vy.setRowVectorUnchecked(j + ROWSA + ROWSB + ROWSC, x4.getRowVectorUnchecked(j));
  }
  return vy;
}

// Rotate the elements of each row of a DSPVectorArray by one element left.
// The first element of each row is moved to the end
template <size_t ROWS>
inline ml::DSPVectorArray<ROWS> rotateLeft(const ml::DSPVectorArray<ROWS>& x)
{
  ml::DSPVectorArray<ROWS> vy;

  for (size_t row = 0; row < ROWS; row++)
  {
    const float* px1 = x.getConstBuffer() + (row * kFloatsPerDSPVector);
    const float* px2 = px1 + kFloatsPerSIMDVector;
    float* py1 = vy.getBuffer() + (row * kFloatsPerDSPVector);

    for (int n = 0; n < kSIMDVectorsPerDSPVector - 1; ++n)
    {
      vecStore(py1, vecShuffleLeft(vecLoad(px1), vecLoad(px2)));

      px1 += kFloatsPerSIMDVector;
      px2 += kFloatsPerSIMDVector;
      py1 += kFloatsPerSIMDVector;
    }

    px2 = x.getConstBuffer() + (row * kFloatsPerDSPVector);

    vecStore(py1, vecShuffleLeft(vecLoad(px1), vecLoad(px2)));
  }

  return vy;
}

// Rotate the elements of each row of a DSPVectorArray by one element right.
// The last element of each row is moved to the start
template <size_t ROWS>
inline ml::DSPVectorArray<ROWS> rotateRight(const ml::DSPVectorArray<ROWS>& x)
{
  ml::DSPVectorArray<ROWS> vy;

  for (size_t row = 0; row < ROWS; row++)
  {
    const float* px1 = x.getConstBuffer() + (row * kFloatsPerDSPVector);
    const float* px2 = px1 + kFloatsPerSIMDVector;
    float* py1 = vy.getBuffer() + (row * kFloatsPerDSPVector) + kFloatsPerSIMDVector;

    for (int n = 0; n < kSIMDVectorsPerDSPVector - 1; ++n)
    {
      vecStore(py1, vecShuffleRight(vecLoad(px1), vecLoad(px2)));

      px1 += kFloatsPerSIMDVector;
      px2 += kFloatsPerSIMDVector;
      py1 += kFloatsPerSIMDVector;
    }

    px2 = x.getConstBuffer() + (row * kFloatsPerDSPVector);
    py1 = vy.getBuffer() + (row * kFloatsPerDSPVector);

    vecStore(py1, vecShuffleRight(vecLoad(px1), vecLoad(px2)));
  }

  return vy;
}

// shuffle two DSPVectorArrays, alternating x1 to even rows of result and x2 to
// odd rows. if the sources are different sizes, the excess rows are all
// appended to the destination after shuffling is done.
template <size_t ROWSA, size_t ROWSB>
inline DSPVectorArray<ROWSA + ROWSB> shuffleRows(const DSPVectorArray<ROWSA> x1,
                                                 const DSPVectorArray<ROWSB> x2)
{
  DSPVectorArray<ROWSA + ROWSB> vy;
  int ja = 0;
  int jb = 0;
  int jy = 0;
  while ((ja < ROWSA) || (jb < ROWSB))
  {
    if (ja < ROWSA)
    {
      vy.setRowVectorUnchecked(jy, x1.getRowVectorUnchecked(ja));
      ja++;
      jy++;
    }
    if (jb < ROWSB)
    {
      vy.setRowVectorUnchecked(jy, x2.getRowVectorUnchecked(jb));
      jb++;
      jy++;
    }
  }
  return vy;
}

// ----------------------------------------------------------------
// separating rows

template <size_t ROWS>
inline DSPVectorArray<(ROWS + 1) / 2> evenRows(const DSPVectorArray<ROWS>& x1)
{
  DSPVectorArray<(ROWS + 1) / 2> vy;
  for (int j = 0; j < (ROWS + 1) / 2; ++j)
  {
    vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(j * 2));
  }
  return vy;
}

template <size_t ROWS>
inline DSPVectorArray<ROWS / 2> oddRows(const DSPVectorArray<ROWS>& x1)
{
  DSPVectorArray<ROWS / 2> vy;
  for (int j = 0; j < ROWS / 2; ++j)
  {
    vy.setRowVectorUnchecked(j, x1.getRowVectorUnchecked(j * 2 + 1));
  }
  return vy;
}

// return the DSPVectorArray consisting of rows [A-B) of the input.
template <size_t A, size_t B, size_t ROWS>
inline DSPVectorArray<B - A> separateRows(const DSPVectorArray<ROWS>& x)
{
  static_assert(B <= ROWS, "separateRows: range out of bounds!");
  static_assert(A < ROWS, "separateRows: range out of bounds!");
  DSPVectorArray<B - A> vy;
  for (int j = A; j < B; ++j)
  {
    vy.setRowVectorUnchecked(j - A, x.getRowVectorUnchecked(j));
  }
  return vy;
}

// ----------------------------------------------------------------
// add rows to get row-wise sum

template <size_t ROWS>
inline DSPVector addRows(const DSPVectorArray<ROWS>& x)
{
  DSPVector vy{0.f};

  for (int j = 0; j < ROWS; ++j)
  {
    vy = add(vy, x.getRowVectorUnchecked(j));
  }
  return vy;
}

// ----------------------------------------------------------------
// rowIndex - returns a DSPVector of j rows, each row filled
// with the index of its row

template <size_t ROWS>
inline DSPVectorArray<ROWS> rowIndex()
{
  DSPVectorArray<ROWS> y;
  for (int j = 0; j < ROWS; ++j)
  {
    y.setRowVectorUnchecked(j, DSPVector(j));
  }
  return y;
}

// ----------------------------------------------------------------
// columnIndex<n> - shorthand for repeatRows<n>(columnIndex())

template <size_t ROWS>
inline DSPVectorArray<ROWS> columnIndex()
{
  return repeatRows<ROWS>(DSPVector(make_array<kFloatsPerDSPVector>(intToFloatCastFn)));
}

// TODO variadic splitRows(bundleSIg, outputRow1, outputRow2, ... )

// ----------------------------------------------------------------
// for testing

template <size_t ROWS>
inline std::ostream& operator<<(std::ostream& out, const DSPVectorArray<ROWS>& vecArray)
{
  //    if(ROWS > 1) out << "[   ";
  for (int v = 0; v < ROWS; ++v)
  {
    //  if(ROWS > 1) if(v > 0) out << "\n    ";
    if (ROWS > 1) out << "\n    v" << v << ": ";
    out << "[";
    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      out << vecArray[v * kFloatsPerDSPVector + i] << " ";
    }
    out << "] ";
  }
  //    if(ROWS > 1) out << "]";
  return out;
}

template <size_t ROWS>
inline std::ostream& operator<<(std::ostream& out, const DSPVectorArrayInt<ROWS>& vecArray)
{
  out << "@" << std::hex << reinterpret_cast<unsigned long>(&vecArray) << std::dec << "\n ";
  //    if(ROWS > 1) out << "[   ";
  for (int v = 0; v < ROWS; ++v)
  {
    if (ROWS > 1)
      if (v > 0) out << "\n    ";
    if (ROWS > 1) out << "v" << v << ": ";
    out << "[";
    for (int i = 0; i < kIntsPerDSPVector; ++i)
    {
      out << vecArray[v * kIntsPerDSPVector + i] << " ";
    }
    out << "] ";
  }
  //    if(ROWS > 1) out << "]";
  return out;
}

inline bool validate(const DSPVector& x)
{
  bool r = true;
  for (int n = 0; n < kFloatsPerDSPVector; ++n)
  {
    const float maxUsefulValue = 1e8;
    if (ml::isNaN(x[n]) || (fabs(x[n]) > maxUsefulValue))
    {
      std::cout << "error: " << x[n] << " at index " << n << "\n";
      std::cout << x << "\n";
      r = false;
      break;
    }
  }
  return r;
}
}  // namespace ml
