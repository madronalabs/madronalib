
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <stdint.h>

#include <cassert>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "MLDSPMath.h"
#include "MLDSPScalarMath.h"
#include "MLText.h"

namespace ml
{
const uintptr_t kSignalAlignBits =
    4;  // cache line is 64 bytes, minimum signal size is one SIMD vector
const uintptr_t kSignalAlignSize = 1 << kSignalAlignBits;
const uintptr_t kSignalAlignMask = ~(kSignalAlignSize - 1);

typedef enum
{
  kLoopType1DEnd = 0
} eLoopType;

constexpr float kTimeless = -1.f;
constexpr float kToBeCalculated = 0.f;

// NOTE: Matrix used to be called Signal, so some of these comments will be
// confusing until rewritten.

// ----------------------------------------------------------------
// A signal. A finite, discrete representation of data we will
// generate, modify, look at listen to, etc.
//
// Signals can have multiple dimensions.  If a signal is marked
// as a time series, the first (most-significant) dimension
// is an index into multiple samples.  Otherwise, the signal has
// the given number of dimensions and no temporal extent.
//
// 1D matrix: either
// time is dim 1, signal is dimensionless (typical audio signal)
// or
// no time, signal is 1D on dim[1]
//
// 2D matrix: either
// time is dim 2, signal is 1D on dims[1] (audio signal in freq. domain)
// or
// no time, signal is 2D on dims[2, 1] (image)
//
//
// A signal always allocates storage in power of 2 sizes.  For signals of
// dimension > 1, bitmasks are used to force accesses to be within bounds.

// Signals greater than three dimensions are used so little, it seems to make
// sense for objects that would need those signals to implement them
// as vectors of 3D signals or some such thing.

class Matrix final
{
 private:
  // ----------------------------------------------------------------
  // data
  // start of aligned data in memory.
  float* mDataAligned;

  // TODO
  // mDataAligned
  // mWidthBits, mHeightBits, mDepthBits, mDataOffset (data -> aligned) (can
  // also compute) that's it NO we need width, height, depth independent of ^2
  // sizes

  // start of signal data in memory.
  // If this is 0, we do not own any data.  However, in the case of a
  // reference to another signal mDataAligned may still refer to external data.
  float* mData;

  // store requested size of each dimension. For 1D signals, height is 1, etc.
  int mWidth, mHeight, mDepth;

  // Sample rate in Hz.  if negative, signal is not a time series.
  // if zero, rate is a positive one that hasn't been calculated by the DSP
  // engine yet.
  // TODO should be int
  float mRate;

  // total power-of-two size in samples, stored for fast access by clear() etc.
  int mSize;

  // log2 of actual size of each dimension, stored for fast access.
  int mWidthBits, mHeightBits, mDepthBits;

  static constexpr int kSmallSignalSize =
      64;  // TODO this should be equal to the fixed DSP vector size

  float mLocalData[kSmallSignalSize + kSignalAlignSize - 1];

 public:
  static Matrix nullSignal;

  explicit Matrix();
  Matrix(const Matrix& b);
  explicit Matrix(int width, int height = 1, int depth = 1);
  explicit Matrix(int width, int height, int depth, const float* pData);
  Matrix(std::initializer_list<float> values);

  // create a looped version of the signal argument, according to the loop type
  Matrix(Matrix src, eLoopType loopType, int loopLength);

  inline Matrix(int width, std::function<float(int)> fillFn)
      : mDataAligned(0), mData(0)
  {
    setDims(width);
    for (int n = 0; n < width; ++n)
    {
      mDataAligned[n] = fillFn(n);
    }
  }

  ~Matrix();
  Matrix& operator=(const Matrix& other);

  inline float* getBuffer(void) const { return mDataAligned; }

  inline const float* getConstBuffer(void) const { return mDataAligned; }

  // 1-D access methods
  //
  // inspector applied to const references, typically from getInput()
  inline float operator[](int i) const
  {
    assert(i < mSize);
    return mDataAligned[i];
  }

  // mutator, called for non-const references
  inline float& operator[](int i)
  {
    assert(i < mSize);
    return mDataAligned[i];
  }

  inline void setToConstant(float k)
  {
    /*
     int c = mSize >> kFloatsPerSIMDVectorBits;
     const __m128 vk = _mm_set1_ps(k);
     float* py1 = mDataAligned;

     for (int n = 0; n < c; ++n)
     {
            _mm_store_ps(py1, vk);
            py1 += kFloatsPerSIMDVector;
     }*/

    // TEMP
    int frames = getSize();
    for (int n = 0; n < frames; ++n)
    {
      mDataAligned[n] = k;
    }
  }

  // return signal value at the position p, interpolated linearly.
  // For power-of-two size tables, this will interpolate around the loop.
  inline float getInterpolatedLinear(float p) const
  {
    int pi = (int)p;
    float m = p - pi;
    int k = mSize - 1;
    float r0 = mDataAligned[pi];
    float r1 = mDataAligned[(pi + 1) & k];
    return ml::lerp(r0, r1, m);
  }

  void addDeinterpolatedLinear(float p, float v)
  {
    // TODO SSE
    int k = mSize - 1;
    float eps = 0.00001f;
    float fw = (float)mWidth - eps;
    float pc = ml::min(p, fw);
    int pi = (int)pc;
    float m = pc - pi;
    mDataAligned[pi] += (1.0f - m) * v;
    mDataAligned[(pi + 1) & k] += (m)*v;
  }

  // 2D access methods
  //
  // mutator, return reference to sample
  // (TODO) does not work with reference?! (Matrix& t = mySignal; t(2, 3) = k;)
  inline float& operator()(const int i, const int j)
  {
    assert((j << mWidthBits) + i < mSize);
    return mDataAligned[(j << mWidthBits) + i];
  }

  // inspector, return by value
  inline const float operator()(const int i, const int j) const
  {
    assert((j << mWidthBits) + i < mSize);
    return mDataAligned[(j << mWidthBits) + i];
  }

  // interpolators could be lambdas?
  // play with ideas and benchmark.

  inline float getInterpolatedLinear(float fi, float fj) const
  {
    float a, b, c, d;

    int i = (int)(fi);
    int j = (int)(fj);

    // get truncate down for inputs < 0
    // TODO use vectors with _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
    // _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
    if (fi < 0) i--;
    if (fj < 0) j--;
    float ri = fi - i;
    float rj = fj - j;

    int i1ok = ml::within(i, 0, mWidth);
    int i2ok = ml::within(i + 1, 0, mWidth);
    int j1ok = ml::within(j, 0, mHeight);
    int j2ok = ml::within(j + 1, 0, mHeight);

    a = (j1ok && i1ok) ? mDataAligned[row(j) + i] : 0.f;
    b = (j1ok && i2ok) ? mDataAligned[row(j) + i + 1] : 0.f;
    c = (j2ok && i1ok) ? mDataAligned[row(j + 1) + i] : 0.f;
    d = (j2ok && i2ok) ? mDataAligned[row(j + 1) + i + 1] : 0.f;

    return ml::lerp(ml::lerp(a, b, ri), ml::lerp(c, d, ri), rj);
  }

  // NOTE: this code below was added more recently but is breaking Soundplane
  // touches.
  // TODO investigate and optimize
  /*
   {
          int i = (int)fi;
          int j = (int)py;
          float mx = fi - i;
          float my = fj - j;
   float r00 = mDataAligned[(row(j) + i)];
   float r01 = mDataAligned[(row(j) + i + 1)];
   float r10 = mDataAligned[(row(j + 1) + i)];
   float r11 = mDataAligned[(row(j + 1) + i + 1)];
   float r0 = lerp(r00, r01, mx);
   float r1 = lerp(r10, r11, mx);
   float r = lerp(r0, r1, my);
          return r;
   }
   */

  inline float cubic(float y0, float y1, float y2, float y3, float m) const
  {
    float mm = m * m;
    float a0 = y3 - y2 - y0 + y1;
    float a1 = y0 - y1 - a0;
    float a2 = y2 - y0;
    float a3 = y1;
    return (a0 * m * mm + a1 * mm + a2 * m + a3);
  }

  inline float getInterpolatedCubic(float fi, float fj) const
  {
    float p00, p01, p02, p03;
    float p10, p11, p12, p13;
    float p20, p21, p22, p23;
    float p30, p31, p32, p33;

    int i = (int)(fi);
    int j = (int)(fj);
    // get truncate down for inputs < 0
    // TODO use vectors with _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
    // _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
    if (fi < 0) i--;
    if (fj < 0) j--;
    float ri = fi - i;
    float rj = fj - j;

    int x0, x1, x2, x3;
    int y0, y1, y2, y3;

    x0 = i - 1;
    x1 = i;
    x2 = i + 1;
    x3 = i + 2;

    y0 = j - 1;
    y1 = j;
    y2 = j + 1;
    y3 = j + 2;

    int w = getWidth();
    int h = getHeight();
    x0 = ml::clamp(x0, 0, w);
    x1 = ml::clamp(x1, 0, w);
    x2 = ml::clamp(x2, 0, w);
    x3 = ml::clamp(x3, 0, w);
    y0 = ml::clamp(y0, 0, h);
    y1 = ml::clamp(y1, 0, h);
    y2 = ml::clamp(y2, 0, h);
    y3 = ml::clamp(y3, 0, h);

    p00 = mDataAligned[row(y0) + x0];
    p01 = mDataAligned[row(y1) + x0];
    p02 = mDataAligned[row(y2) + x0];
    p03 = mDataAligned[row(y3) + x0];

    p10 = mDataAligned[row(y0) + x1];
    p11 = mDataAligned[row(y1) + x1];
    p12 = mDataAligned[row(y2) + x1];
    p13 = mDataAligned[row(y3) + x1];

    p20 = mDataAligned[row(y0) + x2];
    p21 = mDataAligned[row(y1) + x2];
    p22 = mDataAligned[row(y2) + x2];
    p23 = mDataAligned[row(y3) + x2];

    p30 = mDataAligned[row(y0) + x3];
    p31 = mDataAligned[row(y1) + x3];
    p32 = mDataAligned[row(y2) + x3];
    p33 = mDataAligned[row(y3) + x3];

    return cubic(cubic(p00, p01, p02, p03, ri), cubic(p10, p11, p12, p13, ri),
                 cubic(p20, p21, p22, p23, ri), cubic(p30, p31, p32, p33, ri),
                 rj);
  }

  void addDeinterpolatedLinear(float px, float py, float v)
  {
    // TODO SSE
    float eps = 0.00001f;
    float fw = (float)mWidth - eps;
    float fh = (float)mHeight - eps;
    float pxc = ml::min(px, fw);
    float pyc = ml::min(py, fh);
    int pxi = (int)pxc;
    int pyi = (int)pyc;
    float mx = pxc - pxi;
    float my = pyc - pyi;
    float r0 = (1.0f - my) * v;
    float r1 = (my)*v;
    float r00 = (1.0f - mx) * r0;
    float r01 = (mx)*r0;
    float r10 = (1.0f - mx) * r1;
    float r11 = (mx)*r1;
    mDataAligned[(row(pyi) + pxi)] += r00;
    mDataAligned[(row(pyi) + pxi + 1)] += r01;
    mDataAligned[(row(pyi + 1) + pxi)] += r10;
    mDataAligned[(row(pyi + 1) + pxi + 1)] += r11;
  }

  const float operator()(const float i, const float j) const;
  //    const float getInterpolatedLinear(const Vec2& pos) const { return
  //    getInterpolatedLinear(pos.x(), pos.y()); }

  // 3D access methods
  //
  // mutator, return sample reference
  inline float& operator()(const int i, const int j, const int k)
  {
    assert((k << mWidthBits << mHeightBits) + (j << mWidthBits) + i < mSize);
    return mDataAligned[(k << mWidthBits << mHeightBits) + (j << mWidthBits) +
                        i];
  }

  // inspector, return sample by value
  inline const float operator()(const int i, const int j, const int k) const
  {
    assert((k << mWidthBits << mHeightBits) + (j << mWidthBits) + i < mSize);
    return mDataAligned[(k << mWidthBits << mHeightBits) + (j << mWidthBits) +
                        i];
  }

  // getFrame() - return const 2D signal made from data in place.
  const Matrix getFrame(int i) const;

  // setFrame() - set the 2D frame i to the incoming signal.
  void setFrame(int i, const Matrix& src);

  // return 1, 2 or 3 element matrix with dimensions
  Matrix getDims();

  // set dims.  return data ptr, or 0 if out of memory.
  float* setDims(int width, int height = 1, int depth = 1);

  // same but with (w, h, d) signal
  float* setDims(const Matrix& whd);

  // MLRect getBoundsRect() const { return MLRect(0, 0, mWidth, mHeight); }

  int getWidth() const { return mWidth; }
  int getHeight() const { return mHeight; }
  int getDepth() const { return mDepth; }
  int getWidthBits() const { return mWidthBits; }
  int getHeightBits() const { return mHeightBits; }
  int getDepthBits() const { return mDepthBits; }
  inline int getSize() const { return mSize; }

  int getXStride() const { return (int)sizeof(float); }
  int getYStride() const { return (int)sizeof(float) << mWidthBits; }
  int getZStride() const
  {
    return (int)sizeof(float) << mWidthBits << mHeightBits;
  }
  int getFrames() const;

  // rate
  void setRate(float rate) { mRate = rate; }
  float getRate() const { return mRate; }

  // I/O
  void read(const float* input, const int offset, const int n);
  void write(float* output, const int offset, const int n);
  void readFromPackedData(const float* input);
  void writeToPackedData(float* pDest);

  void sigClamp(const Matrix& a, const Matrix& b);
  void sigMin(const Matrix& b);
  void sigMax(const Matrix& b);

  // mix this signal with signal b.
  void sigLerp(const Matrix& b, const float mix);
  void sigLerp(const Matrix& b, const Matrix& mix);

  // binary operators on Signals  TODO rewrite standard
  // TODO should be explicit to prevent e.g. multiply by Matrix(2) when scale(2)
  // was meant
  bool operator==(const Matrix& b) const;
  bool operator!=(const Matrix& b) const { return !(operator==(b)); }
  void copy(const Matrix& b);
  void add(const Matrix& b);
  void subtract(const Matrix& b);
  void multiply(const Matrix& s);
  void divide(const Matrix& s);

  void copyFast(const Matrix& b);

  // signal / scalar operators
  void fill(const float f);
  void scale(const float k);
  void add(const float k);
  void subtract(const float k);
  void subtractFrom(const float k);

  // should these be friends?
  void sigClamp(const float min, const float max);
  void sigMin(const float min);
  void sigMax(const float max);

  // 1D convolution
  void convolve3x1(const float km, const float k, const float kp);
  void convolve5x1(const float kmm, const float km, const float k,
                   const float kp, const float kpp);

  // Convolve the 2D matrix with a radially symmetric 3x3 matrix defined by
  // coefficients kc (center), ke (edge), and kk (corner).
  void convolve3x3r(const float kc, const float ke, const float kk);
  void convolve3x3rb(const float kc, const float ke, const float kk);

  // metrics
  float getRMS();
  float rmsDiff(const Matrix& b);

  // spatial
  void flipVertical();

  // find the subpixel peak in the neighborhood of the input position,
  // by 2D Taylor series expansion of the surface function at (x, y).
  // The result is clamped to the input position plus or minus maxCorrect.
  //
  // Vec2 correctPeak(const int ix, const int iy, const float maxCorrect) const;

  // unary operators on Signals
  void square();
  void sqrt();
  void abs();
  void inv();
  void ssign();
  void exp2();

  // 2D signal utils
  void setIdentity();
  void makeDuplicateBoundary2D();
  void partialDiffX();
  void partialDiffY();

  // return highest value in signal
  // Vec3 findPeak() const;

  // TODO different module
  // add (blit) another 2D signal
  //	void add2D(const Matrix& b, int destX, int destY);
  //	void add2D(const Matrix& b, const Vec2& destOffset);

  inline void clear() { setToConstant(0); }

  void invert();

  int checkIntegrity() const;
  int checkForNaN() const;
  float getSum() const;
  float getMean() const;
  float getMin() const;
  float getMax() const;
  void dump(std::ostream& s, int verbosity = 0) const;

  //	void dump(std::ostream& s, const MLRect& b) const;
  void dumpASCII(std::ostream& s) const;

  inline bool is1D() const
  {
    return ((mWidth > 1) && (mHeight == 1) && (mDepth == 1));
  }
  inline bool is2D() const
  {
    return ((mWidth > 1) && (mHeight > 1) && (mDepth == 1));
  }
  inline bool is3D() const
  {
    return ((mWidth > 1) && (mHeight > 1) && (mDepth > 1));
  }

  // handy shorthand for row and plane access
  // TODO looking at actual use, would look better to return dataAligned + row,
  // plane.
  inline int row(int i) const { return i << mWidthBits; }
  inline int plane(int i) const { return i << mWidthBits << mHeightBits; }
  inline int getRowStride() const { return 1 << mWidthBits; }
  inline int getPlaneStride() const { return 1 << mWidthBits << mHeightBits; }

  inline Matrix getRow(int i)
  {
    int w = getWidth();
    Matrix r(w);
    // r.copyFrom( mDataAligned+row(i) );
    float* pRow = mDataAligned + row(i);
    std::copy(pRow, pRow + w, r.mDataAligned);
    return r;
  }

  /*
   inline void scaleAndAccumulate(const Matrix& b, float k)
   {
          int vectors = mSize >> kfloatsPerSIMDVectorBits;

          float* pb = b.getBuffer();
          float* pa = getBuffer();
          __m128 va, vb, vk;

          vk = _mm_set1_ps(k);

          for(int v=0; v<vectors; ++v)
          {
   va = _mm_load_ps(pa);
   vb = _mm_load_ps(pb);
   _mm_store_ps(pa, _mm_add_ps(va, _mm_mul_ps(vb, vk)));
   pa += kFloatsPerSIMDVector;
   pb += kFloatsPerSIMDVector;
          }
   }
   */

  // utilities for getting pointers to the aligned data as other types.
  uint32_t* asUInt32Ptr(void) const
  {
    return reinterpret_cast<uint32_t*>(mDataAligned);
  }
  const uint32_t* asConstUInt32Ptr(void) const
  {
    return reinterpret_cast<const uint32_t*>(mDataAligned);
  }
  int32_t* asInt32Ptr(void) const
  {
    return reinterpret_cast<int32_t*>(mDataAligned);
  }
  const int32_t* asConstInt32Ptr(void) const
  {
    return reinterpret_cast<const int32_t*>(mDataAligned);
  }
  __m128* asM128Ptr(void) const
  {
    return reinterpret_cast<__m128*>(mDataAligned);
  }
  const __m128* asConstM128Ptr(void) const
  {
    return reinterpret_cast<const __m128*>(mDataAligned);
  }
  __m128i* asM128IPtr(void) const
  {
    return reinterpret_cast<__m128i*>(mDataAligned);
  }
  const __m128i* asConstM128IPtr(void) const
  {
    return reinterpret_cast<const __m128i*>(mDataAligned);
  }

  // helper functions
  static Matrix copyWithLoopAtEnd(const Matrix& src, int loopLength);

 private:
  // private signal constructor: make a reference to a frame of the external
  // signal.
  Matrix(const Matrix* other, int frame);

  inline float* allocateData(int size)
  {
    mSize = size;
    if (size <= kSmallSignalSize)
    {
      return mLocalData;
    }

    float* newData = new float[size + kSignalAlignSize - 1];
    if (!newData) mSize = 0;
    return newData;
  }

  inline void freeData()
  {
    if (!(mData == mLocalData))
    {
      delete[] mData;
      mData = nullptr;
      mDataAligned = nullptr;
    }
  }

  inline float* alignToSignal(const float* p)
  {
    uintptr_t pM = (uintptr_t)p;
    pM += (uintptr_t)(kSignalAlignSize - 1);
    pM &= kSignalAlignMask;
    return (float*)pM;
  }

  inline float* initializeData(float* pData, int size)
  {
    float* newDataAligned = 0;
    if (pData)
    {
      newDataAligned = alignToSignal(pData);
      memset((void*)(newDataAligned), 0, (size_t)(size * sizeof(float)));
    }
    return newDataAligned;
  }
};

float rmsDifference2D(const Matrix& a, const Matrix& b);

#pragma mark new business

inline Matrix add(const Matrix& a, const Matrix& b)
{
  Matrix r(a);
  r.add(b);
  return r;
}

inline Matrix clampSignal(const Matrix& x, float a, float b)
{
  Matrix r(a);
  r.sigClamp(a, b);
  return r;
}

// return the matrix transpose of a 1D or 2D signal.
inline Matrix transpose(const Matrix& x)
{
  // this whole thing could be something like
  // return Matrix(float* pDataStart, int w, int h, int rowStride, int
  // colStride);
  int yh = x.getWidth();
  int yw = x.getHeight();
  Matrix y(yw, yh);
  for (int j = 0; j < yh; ++j)
  {
    for (int i = 0; i < yw; ++i)
    {
      y(i, j) = x(j, i);
    }
  }
  return y;
}

inline Matrix matrixMultiply2D(Matrix A, Matrix B)
{
  if (A.getWidth() != B.getHeight())
  {
    return Matrix::nullSignal;
  }

  int h = A.getHeight();
  int w = B.getWidth();
  int m = A.getWidth();
  Matrix AB(w, h);

  for (int j = 0; j < h; ++j)
  {
    for (int i = 0; i < w; ++i)
    {
      float ijSum = 0.f;
      for (int k = 0; k < m; ++k)
      {
        ijSum += A(k, j) * B(i, k);
      }
      AB(i, j) = ijSum;
    }
  }
  return AB;
}

std::ostream& operator<<(std::ostream& out, const ml::Matrix& r);

}  // namespace ml
