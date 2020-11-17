// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// DSP filters: functor objects implementing an operator()(DSPVector input, ...).
// All these filters have some state, otherwise they would be DSPOps.
//
// These objects are for building fixed DSP graphs in a functional style. The
// compiler should have many opportunities to optimize these graphs. For dynamic
// graphs changeable at runtime, see MLProcs. In general MLProcs will be written
// using DSPGens, DSPOps, DSPFilters.
//
// Filter cutoffs are set by a parameter omega, equal to frequency / sample
// rate. This lets filter objects be unaware of the sample rate, resulting in
// less code overall. For all filters, k is a damping parameter equal to 1/Q
// where Q is the analog filter "quality." For bell and shelf filters, gain is
// specified as an output / input ratio A.

#pragma once

#include <vector>

#include "MLDSPGens.h"

namespace ml
{
// use this, not dBToAmp for calculating filter gain parameter A.
inline float dBToGain(float dB) { return powf(10.f, dB / 40.f); }

// from a coefficients start array and a coefficients end array, make a
// DSPVectorArray with each coefficient interpolated over time.
template <size_t COEFFS_SIZE>
DSPVectorArray<COEFFS_SIZE> interpolateCoeffsLinear(const std::array<float, COEFFS_SIZE> c0,
                                                    const std::array<float, COEFFS_SIZE> c1)
{
  DSPVectorArray<COEFFS_SIZE> vy;
  for (int i = 0; i < COEFFS_SIZE; ++i)
  {
    vy.row(i) = interpolateDSPVectorLinear(c0[i], c1[i]);
  }
  return vy;
}

// --------------------------------------------------------------------------------
// utility filters implemented as SVF variations
// Thanks to Andrew Simper [www.cytomic.com] for sharing his work over the
// years.

class Lopass
{
  struct _coeffs
  {
    float g0, g1, g2;
  };

  float ic1eq{0};
  float ic2eq{0};

 public:
  _coeffs mCoeffs{0};

  static _coeffs coeffs(float omega, float k)
  {
    float piOmega = kPi * omega;
    float s1 = sinf(piOmega);
    float s2 = sinf(2.0f * piOmega);
    float nrm = 1.0f / (2.f + k * s2);
    float g0 = s2 * nrm;
    float g1 = (-2.f * s1 * s1 - k * s2) * nrm;
    float g2 = (2.0f * s1 * s1) * nrm;
    return {g0, g1, g2};
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float t0 = v0 - ic2eq;
      float t1 = mCoeffs.g0 * t0 + mCoeffs.g1 * ic1eq;
      float t2 = mCoeffs.g2 * t0 + mCoeffs.g0 * ic1eq;
      float v2 = t2 + ic2eq;
      ic1eq += 2.0f * t1;
      ic2eq += 2.0f * t2;
      vy[n] = v2;
    }
    return vy;
  }
};

class Hipass
{
  struct _coeffs
  {
    float g0, g1, g2, k;
  };

  float ic1eq{0};
  float ic2eq{0};

 public:
  _coeffs mCoeffs{0};

  static _coeffs coeffs(float omega, float k)
  {
    float piOmega = kPi * omega;
    float s1 = sinf(piOmega);
    float s2 = sinf(2.0f * piOmega);
    float nrm = 1.0f / (2.f + k * s2);
    float g0 = s2 * nrm;
    float g1 = (-2.f * s1 * s1 - k * s2) * nrm;
    float g2 = (2.0f * s1 * s1) * nrm;
    return {g0, g1, g2, k};
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float t0 = v0 - ic2eq;
      float t1 = mCoeffs.g0 * t0 + mCoeffs.g1 * ic1eq;
      float t2 = mCoeffs.g2 * t0 + mCoeffs.g0 * ic1eq;
      float v1 = t1 + ic1eq;
      float v2 = t2 + ic2eq;
      ic1eq += 2.0f * t1;
      ic2eq += 2.0f * t2;
      vy[n] = v0 - mCoeffs.k * v1 - v2;
    }
    return vy;
  }
};

class Bandpass
{
  struct _coeffs
  {
    float g0, g1, g2;
  };

  float ic1eq{0};
  float ic2eq{0};

 public:
  _coeffs mCoeffs{0};

  static _coeffs coeffs(float omega, float k)
  {
    float piOmega = kPi * omega;
    float s1 = sinf(piOmega);
    float s2 = sinf(2.0f * piOmega);
    float nrm = 1.0f / (2.f + k * s2);
    float g0 = s2 * nrm;
    float g1 = (-2.f * s1 * s1 - k * s2) * nrm;
    float g2 = (2.0f * s1 * s1) * nrm;
    return {g0, g1, g2};
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float t0 = v0 - ic2eq;
      float t1 = mCoeffs.g0 * t0 + mCoeffs.g1 * ic1eq;
      float t2 = mCoeffs.g2 * t0 + mCoeffs.g0 * ic1eq;
      float v1 = t1 + ic1eq;
      ic1eq += 2.0f * t1;
      ic2eq += 2.0f * t2;
      vy[n] = v1;
    }
    return vy;
  }
};

class LoShelf
{
  enum coeffNames
  {
    a1,
    a2,
    a3,
    m1,
    m2,
    COEFFS_SIZE
  };
  typedef std::array<float, COEFFS_SIZE> _coeffs;
  typedef DSPVectorArray<COEFFS_SIZE> _vcoeffs;

  float ic1eq{0};
  float ic2eq{0};

 public:
  enum paramNames
  {
    omega,
    k,
    A,
    PARAMS_SIZE
  };
  typedef std::array<float, PARAMS_SIZE> params;
  _coeffs mCoeffs{};

  static _coeffs coeffs(params p)
  {
    _coeffs r;
    float piOmega = kPi * p[omega];
    float g = tanf(piOmega) / sqrtf(p[A]);
    r[a1] = 1.f / (1.f + g * (g + p[k]));
    r[a2] = g * r[a1];
    r[a3] = g * r[a2];
    r[m1] = p[k] * (p[A] - 1.f);
    r[m2] = (p[A] * p[A] - 1.f);
    return r;
  }

  static _vcoeffs vcoeffs(const params p0, const params p1)
  {
    return interpolateCoeffsLinear(coeffs(p0), coeffs(p1));
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float v3 = v0 - ic2eq;
      float v1 = mCoeffs[a1] * ic1eq + mCoeffs[a2] * v3;
      float v2 = ic2eq + mCoeffs[a2] * ic1eq + mCoeffs[a3] * v3;
      ic1eq = 2 * v1 - ic1eq;
      ic2eq = 2 * v2 - ic2eq;
      vy[n] = v0 + mCoeffs[m1] * v1 + mCoeffs[m2] * v2;
    }
    return vy;
  }

  inline DSPVector operator()(const DSPVector vx, const _vcoeffs vc)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float v3 = v0 - ic2eq;
      float v1 = vc.constRow(a1)[n] * ic1eq + vc.constRow(a2)[n] * v3;
      float v2 = ic2eq + vc.constRow(a2)[n] * ic1eq + vc.constRow(a3)[n] * v3;
      ic1eq = 2 * v1 - ic1eq;
      ic2eq = 2 * v2 - ic2eq;
      vy[n] = v0 + vc.constRow(m1)[n] * v1 + vc.constRow(m2)[n] * v2;
    }
    return vy;
  }
};

class HiShelf
{
  enum coeffnames
  {
    a1,
    a2,
    a3,
    m0,
    m1,
    m2,
    COEFFS_SIZE
  };
  typedef std::array<float, COEFFS_SIZE> _coeffs;
  typedef DSPVectorArray<COEFFS_SIZE> _vcoeffs;

  float ic1eq{0};
  float ic2eq{0};

 public:
  enum paramnames
  {
    omega,
    k,
    A,
    PARAMS_SIZE
  };
  typedef std::array<float, PARAMS_SIZE> params;
  _coeffs mCoeffs{};

  static _coeffs coeffs(params p)
  {
    _coeffs r;
    float piOmega = kPi * p[omega];
    float g = tanf(piOmega) * sqrtf(p[A]);
    r[a1] = 1.f / (1.f + g * (g + p[k]));
    r[a2] = g * r[a1];
    r[a3] = g * r[a2];
    r[m0] = p[A] * p[A];
    r[m1] = p[k] * (1.f - p[A]) * p[A];
    r[m2] = (1.f - p[A] * p[A]);
    return r;
  }

  static _vcoeffs vcoeffs(const params p0, const params p1)
  {
    return interpolateCoeffsLinear(coeffs(p0), coeffs(p1));
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float v3 = v0 - ic2eq;
      float v1 = mCoeffs[a1] * ic1eq + mCoeffs[a2] * v3;
      float v2 = ic2eq + mCoeffs[a2] * ic1eq + mCoeffs[a3] * v3;
      ic1eq = 2 * v1 - ic1eq;
      ic2eq = 2 * v2 - ic2eq;
      vy[n] = mCoeffs[m0] * v0 + mCoeffs[m1] * v1 + mCoeffs[m2] * v2;
    }
    return vy;
  }

  inline DSPVector operator()(const DSPVector vx, const _vcoeffs vc)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float v3 = v0 - ic2eq;
      float v1 = vc.constRow(a1)[n] * ic1eq + vc.constRow(a2)[n] * v3;
      float v2 = ic2eq + vc.constRow(a2)[n] * ic1eq + vc.constRow(a3)[n] * v3;
      ic1eq = 2 * v1 - ic1eq;
      ic2eq = 2 * v2 - ic2eq;
      vy[n] = vc.constRow(m0)[n] * v0 + vc.constRow(m1)[n] * v1 + vc.constRow(m2)[n] * v2;
    }
    return vy;
  }
};

class Bell
{
  struct _coeffs
  {
    float a1, a2, a3, m1;
  };

  float ic1eq{0};
  float ic2eq{0};

 public:
  _coeffs mCoeffs{0};

  static _coeffs coeffs(float omega, float k, float A)
  {
    float kc = k / A;  // correct k
    float piOmega = kPi * omega;
    float g = tanf(piOmega);
    float a1 = 1.f / (1.f + g * (g + kc));
    float a2 = g * a1;
    float a3 = g * a2;
    float m1 = kc * (A * A - 1.f);
    return {a1, a2, a3, m1};
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float v3 = v0 - ic2eq;
      float v1 = mCoeffs.a1 * ic1eq + mCoeffs.a2 * v3;
      float v2 = ic2eq + mCoeffs.a2 * ic1eq + mCoeffs.a3 * v3;
      ic1eq = 2 * v1 - ic1eq;
      ic2eq = 2 * v2 - ic2eq;
      vy[n] = v0 + mCoeffs.m1 * v1;
    }
    return vy;
  }
};

// A one pole filter. see https://ccrma.stanford.edu/~jos/fp/One_Pole.html

class OnePole
{
  struct _coeffs
  {
    float a0, b1;
  };

  float y1{0};

 public:
  _coeffs mCoeffs{0};

  static _coeffs coeffs(float omega)
  {
    float x = expf(-omega * kTwoPi);
    return {1.f - x, x};
  }

  static _coeffs passthru() { return {1.f, 0.f}; }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      y1 = mCoeffs.a0 * vx[n] + mCoeffs.b1 * y1;
      vy[n] = y1;
    }
    return vy;
  }
};

// A one-pole, one-zero filter to attenuate DC.
// Works well, but beware of its effects on bass sounds. An omega of 0.05 is a
// good starting point. see https://ccrma.stanford.edu/~jos/fp/DC_Blocker.html
// for more.

class DCBlocker
{
  typedef float _coeffs;
  float x1{0};
  float y1{0};

 public:
  _coeffs mCoeffs{};

  static _coeffs coeffs(float omega) { return cosf(omega); }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      const float x0 = vx[n];
      const float y0 = x0 - x1 + mCoeffs * y1;
      y1 = y0;
      x1 = x0;
      vy[n] = y0;
    }
    return vy;
  }
};

// Differentiator

class Differentiator
{
  float x1{0};

 public:
  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    vy[0] = x1 - vx[0];

    // TODO SIMD
    for (int n = 1; n < kFloatsPerDSPVector; ++n)
    {
      vy[n] = vx[n - 1] - vx[n];
    }
    x1 = vx[kFloatsPerDSPVector];
    return vy;
  }
};

// Integrator

class Integrator
{
  float y1{0};

 public:
  // set leak to a value such as 0.001 for stability
  float mLeak{0};

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      y1 -= y1 * mLeak;
      y1 += vx[n];
      vy[n] = y1;
    }
    return vy;
  }
};

// Peak with exponential decay

class Peak
{
  struct _coeffs
  {
    float a0, b1;
  };

  float y1{0};
  int peakHoldCounter{0};

 public:
  _coeffs mCoeffs{0};
  int peakHoldSamples{44100};

  static _coeffs coeffs(float omega)
  {
    float x = expf(-omega * kTwoPi);
    return {1.f - x, x};
  }

  static _coeffs passthru() { return {1.f, 0.f}; }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    DSPVector vxSquared = vx * vx;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      if (vxSquared[n] > y1)
      {
        // set peak and reset counter
        y1 = vxSquared[n];
        peakHoldCounter = peakHoldSamples;
      }
      else
      {
        // decay
        if (peakHoldCounter <= 0)
        {
          y1 = mCoeffs.a0 * vxSquared[n] + mCoeffs.b1 * y1;
        }
      }
      vy[n] = y1;
    }

    if (peakHoldCounter > 0)
    {
      peakHoldCounter -= kFloatsPerDSPVector;
    }
    return sqrt(vy);
  }
};

// filtered RMS

class RMS
{
  struct _coeffs
  {
    float a0, b1;
  };

  float y1{0};

 public:
  _coeffs mCoeffs{0};

  static _coeffs coeffs(float omega)
  {
    float x = expf(-omega * kTwoPi);
    return {1.f - x, x};
  }

  static _coeffs passthru() { return {1.f, 0.f}; }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    DSPVector vxSquared = vx * vx;

    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      y1 = mCoeffs.a0 * vxSquared[n] + mCoeffs.b1 * y1;
      vy[n] = y1;
    }

    return sqrt(vy);
  }
};

// IntegerDelay delays a signal a whole number of samples.

class IntegerDelay
{
  std::vector<float> mBuffer;
  int mIntDelayInSamples{0};
  uintptr_t mWriteIndex{0};
  uintptr_t mLengthMask{0};

 public:
  IntegerDelay() = default;
  IntegerDelay(int d)
  {
    setMaxDelayInSamples(d);
    setDelayInSamples(d);
  }
  ~IntegerDelay() = default;

  // for efficiency, no bounds checking is done. Because mLengthMask is used to
  // constrain all reads, bad values here may make bad sounds (buffer wraps) but
  // will not attempt to read from outside the buffer.
  inline void setDelayInSamples(int d) { mIntDelayInSamples = d; }

  void setMaxDelayInSamples(float d)
  {
    int dMax = floorf(d);
    int newSize = 1 << bitsToContain(dMax + kFloatsPerDSPVector);
    mBuffer.resize(newSize);
    mLengthMask = newSize - 1;
    mWriteIndex = 0;
    clear();
  }

  inline void clear() { std::fill(mBuffer.begin(), mBuffer.end(), 0.f); }

  inline DSPVector operator()(const DSPVector vx)
  {
    // write
    uintptr_t writeEnd = mWriteIndex + kFloatsPerDSPVector;
    if (writeEnd <= mLengthMask + 1)
    {
      const float* srcStart = vx.getConstBuffer();
      std::copy(srcStart, srcStart + kFloatsPerDSPVector, mBuffer.data() + mWriteIndex);
    }
    else
    {
      uintptr_t excess = writeEnd - mLengthMask - 1;
      const float* srcStart = vx.getConstBuffer();
      const float* srcSplice = srcStart + kFloatsPerDSPVector - excess;
      const float* srcEnd = srcStart + kFloatsPerDSPVector;
      std::copy(srcStart, srcSplice, mBuffer.data() + mWriteIndex);
      std::copy(srcSplice, srcEnd, mBuffer.data());
    }

    // read
    DSPVector vy;
    uintptr_t readStart = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
    uintptr_t readEnd = readStart + kFloatsPerDSPVector;
    float* srcBuf = mBuffer.data();
    if (readEnd <= mLengthMask + 1)
    {
      std::copy(srcBuf + readStart, srcBuf + readEnd, vy.getBuffer());
    }
    else
    {
      uintptr_t excess = readEnd - mLengthMask - 1;
      uintptr_t readSplice = readStart + kFloatsPerDSPVector - excess;
      float* pDest = vy.getBuffer();
      std::copy(srcBuf + readStart, srcBuf + readSplice, pDest);
      std::copy(srcBuf, srcBuf + excess, pDest + (kFloatsPerDSPVector - excess));
    }

    // update index
    mWriteIndex += kFloatsPerDSPVector;
    mWriteIndex &= mLengthMask;
    return vy;
  }

  inline DSPVector operator()(const DSPVector x, const DSPVector delay)
  {
    DSPVector y;

    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      // write
      mBuffer[mWriteIndex] = x[n];

      // read
      mIntDelayInSamples = static_cast<int>(delay[n]);
      uintptr_t readIndex = (mWriteIndex - mIntDelayInSamples) & mLengthMask;

      y[n] = mBuffer[readIndex];
      mWriteIndex++;
      mWriteIndex &= mLengthMask;
    }

    return y;
  }

  inline float processSample(float x)
  {
    // write
    // note that, for performance, there is no bounds checking. If you crash
    // here, you probably didn't allocate enough delay memory.
    mBuffer[mWriteIndex] = x;

    // read
    uintptr_t readIndex = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
    float y = mBuffer[readIndex];

    // update index
    mWriteIndex++;
    mWriteIndex &= mLengthMask;
    return y;
  }
};

// First order allpass section with a single sample of delay.

class Allpass1
{
 private:
  float x1{0}, y1{0};

 public:
  float mCoeffs;

  Allpass1() : mCoeffs(0.f) {}
  Allpass1(float a) : mCoeffs(a) {}
  ~Allpass1() {}

  inline void clear()
  {
    x1 = 0.f;
    y1 = 0.f;
  }

  // get allpass coefficient from a delay fraction d.
  // to minimize modulation noise, d should be in the range [0.618 - 1.618].
  static float coeffs(float d)
  {
    // return 2nd order approx around 1 to (1.f - d) / (1.f + d)
    float xm1 = (d - 1.f);
    return -0.53f * xm1 + 0.24f * xm1 * xm1;
  }

  inline float processSample(const float x)
  {
    // one-multiply form. see
    // https://ccrma.stanford.edu/~jos/pasp/One_Multiply_Scattering_Junctions.html
    float y = x1 + (x - y1) * mCoeffs;
    x1 = x;
    y1 = y;
    return y;
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      vy[n] = processSample(vx[n]);
    }
    return vy;
  }
};

// Combining the integer delay and first order allpass section
// gives us an allpass-interpolated fractional delay. In general, modulating the
// delay time will change the allpass coefficient, producing clicks in the
// output.

class FractionalDelay
{
  IntegerDelay mIntegerDelay;
  Allpass1 mAllpassSection;
  float mDelayInSamples{};

 public:
  FractionalDelay() = default;
  FractionalDelay(float d)
  {
    setMaxDelayInSamples(d);
    setDelayInSamples(d);
  }
  ~FractionalDelay() = default;

  inline void clear()
  {
    mIntegerDelay.clear();
    mAllpassSection.clear();
  }

  inline void setDelayInSamples(float d)
  {
    mDelayInSamples = d;
    float fDelayInt = floorf(d);
    int delayInt = fDelayInt;
    float delayFrac = d - fDelayInt;

    // constrain D to [0.618 - 1.618] if possible
    if ((delayFrac < 0.618f) && (delayInt > 0))
    {
      delayFrac += 1.f;
      delayInt -= 1;
    }
    mIntegerDelay.setDelayInSamples(delayInt);
    mAllpassSection.mCoeffs = Allpass1::coeffs(delayFrac);
  }

  inline void setMaxDelayInSamples(float d) { mIntegerDelay.setMaxDelayInSamples(floorf(d)); }

  // return the input signal, delayed by the constant delay time
  // mDelayInSamples.
  inline DSPVector operator()(const DSPVector vx) { return mAllpassSection(mIntegerDelay(vx)); }

  // return the input signal, delayed by the varying delay time vDelayInSamples.
  inline DSPVector operator()(const DSPVector vx, const DSPVector vDelayInSamples)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      setDelayInSamples(vDelayInSamples[n]);
      vy[n] = mAllpassSection.processSample(mIntegerDelay.processSample(vx[n]));
    }
    return vy;
  }

  // return the input signal, delayed by the varying delay time vDelayInSamples,
  // but only allow changes to the delay time when vChangeTicks is nonzero.
  inline DSPVector operator()(const DSPVector vx, const DSPVector vDelayInSamples,
                              const DSPVectorInt vChangeTicks)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      if (vChangeTicks[n] != 0)
      {
        setDelayInSamples(vDelayInSamples[n]);
      }

      vy[n] = mAllpassSection.processSample(mIntegerDelay.processSample(vx[n]));
    }
    return vy;
  }
};

// Crossfading two allpass-interpolated delays allows modulating the delay
// time without clicks. See "A Lossless, Click-free, Pitchbend-able Delay Line
// Loop Interpolation Scheme", Van Duyne, Jaffe, Scandalis, Stilson, ICMC 1997.

namespace PitchbendableDelayConsts
{
// period in samples of allpass fade cycle. must be a power of 2 less than or
// equal to kFloatsPerDSPVector. 32 sounds good.
constexpr int kFadePeriod{32};
constexpr int fadeRamp(int n) { return n % kFadePeriod; }
constexpr int ticks1(int n) { return fadeRamp(n) == kFadePeriod / 2; }
constexpr int ticks2(int n) { return fadeRamp(n) == 0; }
constexpr float fadeFn(int n)
{
  // triangle from 0 to 1 to 0
  return 2.f * ((fadeRamp(n)) > kFadePeriod / 2 ? 1.0f - (fadeRamp(n)) / (kFadePeriod + 0.f)
                                                : (fadeRamp(n)) / (kFadePeriod + 0.f));
}

// generate vectors of ticks indicating when delays can change
// equality operators on vectors return 0 or 0xFFFFFFFF
// note: mDelay1's delay time will be 0 when the object is created and before
// the first half fade period. so there is a warmup time of one half fade
// period: any input before this will be attenuated.
//
// constexpr fill is used. unfortunately this cannot be made to work with a
// lambda in C++11. TODO Revisit.
constexpr DSPVectorInt test1(fadeRamp);
constexpr DSPVectorInt kvDelay1Changes(ticks1);
constexpr DSPVectorInt kvDelay2Changes(ticks2);
constexpr DSPVector kvFade(fadeFn);
};  // namespace PitchbendableDelayConsts

class PitchbendableDelay
{
  FractionalDelay mDelay1, mDelay2;

 public:
  PitchbendableDelay() = default;

  inline void setMaxDelayInSamples(float d)
  {
    mDelay1.setMaxDelayInSamples(d);
    mDelay2.setMaxDelayInSamples(d);
  }

  inline void clear()
  {
    mDelay1.clear();
    mDelay2.clear();
  }

  inline DSPVector operator()(const DSPVector vInput, const DSPVector vDelayInSamples)
  {
    using namespace PitchbendableDelayConsts;

    // run the fractional delays and crossfade the results.
    return lerp(mDelay1(vInput, vDelayInSamples, kvDelay1Changes),
                mDelay2(vInput, vDelayInSamples, kvDelay2Changes), kvFade);
  }
};

// General purpose allpass filter with arbitrary delay length.
// For efficiency, the minimum delay time is one DSPVector.

template <typename DELAY_TYPE>
class Allpass
{
  DELAY_TYPE mDelay;
  DSPVector vy1{};

 public:
  float mGain{0.f};

  // use setDelayInSamples to set a constant delay time with DELAY_TYPE of
  // IntegerDelay or FractionalDelay.
  inline void setDelayInSamples(float d) { mDelay.setDelayInSamples(d - kFloatsPerDSPVector); }

  inline void setMaxDelayInSamples(float d)
  {
    mDelay.setMaxDelayInSamples(d - kFloatsPerDSPVector);
  }

  inline void clear()
  {
    mDelay.clear();
    vy1 = DSPVector();
  }

  // use with constant delay time.
  inline DSPVector operator()(const DSPVector vInput)
  {
    DSPVector vGain(-mGain);
    DSPVector vDelayInput = vInput - vy1 * vGain;
    DSPVector y = vDelayInput * vGain + vy1;
    vy1 = mDelay(vDelayInput);
    return y;
  }

  // use vDelayInSamples parameter to set a varying delay time with DELAY_TYPE =
  // PitchbendableDelay.
  inline DSPVector operator()(const DSPVector vInput, const DSPVector vDelayInSamples)
  {
    DSPVector vGain(-mGain);
    DSPVector vDelayInput = vInput - vy1 * vGain;
    DSPVector y = vDelayInput * vGain + vy1;
    vy1 = mDelay(vDelayInput, vDelayInSamples - DSPVector(kFloatsPerDSPVector));
    return y;
  }
};

// FDN
// A general Feedback Delay Network with N delay lines connected in an NxN
// matrix.
// TODO DELAY_TYPE parameter for modulation?

template <int SIZE>
class FDN
{
  std::array<IntegerDelay, SIZE> mDelays;
  std::array<OnePole, SIZE> mFilters;
  std::array<DSPVector, SIZE> mDelayInputVectors{{{DSPVector(0.f)}}};

 public:
  // feedback gains array is public—just copy values to set.
  std::array<float, SIZE> mFeedbackGains{{0}};

  void setDelaysInSamples(std::array<float, SIZE> times)
  {
    for (int n = 0; n < SIZE; ++n)
    {
      // we have one DSPVector feedback latency, so compensate delay times for
      // that.
      int len = times[n] - kFloatsPerDSPVector;
      len = max(1, len);
      mDelays[n].setDelayInSamples(len);
    }
  }

  void setFilterCutoffs(std::array<float, SIZE> omegas)
  {
    for (int n = 0; n < SIZE; ++n)
    {
      mFilters[n].mCoeffs = ml::OnePole::coeffs(omegas[n]);
    }
  }

  // stereo output function
  // TODO generalize n-channel output function somehow
  DSPVectorArray<2> operator()(const DSPVector x)
  {
    // run delays, getting DSPVector for each delay
    for (int n = 0; n < SIZE; ++n)
    {
      mDelayInputVectors[n] = mDelays[n](mDelayInputVectors[n]);
    }

    // get output sum
    DSPVector sumR, sumL;
    for (int n = 0; n < (SIZE & (~1)); ++n)
    {
      if (n & 1)
      {
        sumL += mDelayInputVectors[n];
      }
      else
      {
        sumR += mDelayInputVectors[n];
      }
    }

    // inputs = input gains*input sample + filters(M*delay outputs)
    // The feedback matrix M is a unit-gain Householder matrix, which is just
    // the identity matrix minus a constant k, where k = 2/size. Since
    // multiplying this can be simplified so much, you just see a few operations
    // here, not a general matrix multiply.

    DSPVector sumOfDelays;
    for (int n = 0; n < SIZE; ++n)
    {
      sumOfDelays += mDelayInputVectors[n];
    }
    sumOfDelays *= DSPVector(2.0f / SIZE);

    for (int n = 0; n < SIZE; ++n)
    {
      mDelayInputVectors[n] -= (sumOfDelays);
      mDelayInputVectors[n] = mFilters[n](mDelayInputVectors[n]) * DSPVector(mFeedbackGains[n]);
      mDelayInputVectors[n] += x;
    }

    return concatRows(sumL, sumR);
  }
};

// Half Band Filter
// Polyphase allpass filter used to upsample or downsample a signal by 2x.
// Structure due to fred harris, A. G. Constantinides and Valenzuela.

class HalfBandFilter
{
 public:
  inline DSPVector upsampleFirstHalf(const DSPVector vx)
  {
    DSPVector vy;
    int i2 = 0;
    for (int i = 0; i < kFloatsPerDSPVector / 2; ++i)
    {
      vy[i2++] = apa1.processSample(apa0.processSample(vx[i]));
      vy[i2++] = apb1.processSample(apb0.processSample(vx[i]));
    }
    return vy;
  }

  inline DSPVector upsampleSecondHalf(const DSPVector vx)
  {
    DSPVector vy;
    int i2 = 0;
    for (int i = kFloatsPerDSPVector / 2; i < kFloatsPerDSPVector; ++i)
    {
      vy[i2++] = apa1.processSample(apa0.processSample(vx[i]));
      vy[i2++] = apb1.processSample(apb0.processSample(vx[i]));
    }
    return vy;
  }

  inline DSPVector downsample(const DSPVector vx1, const DSPVector vx2)
  {
    DSPVector vy;
    int i2 = 0;
    for (int i = 0; i < kFloatsPerDSPVector / 2; ++i)
    {
      float a0 = apa1.processSample(apa0.processSample(vx1[i2]));
      float b0 = apb1.processSample(apb0.processSample(vx1[i2 + 1]));
      vy[i] = (a0 + b1) * 0.5f;
      b1 = b0;
      i2 += 2;
    }
    i2 = 0;
    for (int i = kFloatsPerDSPVector / 2; i < kFloatsPerDSPVector; ++i)
    {
      float a0 = apa1.processSample(apa0.processSample(vx2[i2]));
      float b0 = apb1.processSample(apb0.processSample(vx2[i2 + 1]));
      vy[i] = (a0 + b1) * 0.5f;
      b1 = b0;
      i2 += 2;
    }
    return vy;
  }

 private:
  // order=4, rejection=70dB, transition band=0.1.
  Allpass1 apa0{0.07986642623635751f}, apa1{0.5453536510711322f}, apb0{0.28382934487410993f},
      apb1{0.8344118914807379f};
  float b1{0};
};

// Downsampler
// a cascade of half band filters, one for each octave.

class Downsampler
{
  std::vector<HalfBandFilter> _filters;
  std::vector<float> _buffers;
  int _octaves;
  int _numBuffers;
  int _bufferSizeInFloats;
  uint32_t _counter{0};

  float* bufferPtr(int idx, int channel)
  {
    return _buffers.data() + idx * _bufferSizeInFloats + kFloatsPerDSPVector * channel;
  }

 public:
  Downsampler(int channels, int octavesDown) : _octaves(octavesDown)
  {
    if (_octaves)
    {
      // one pair of buffers for each octave plus one output buffer.
      _numBuffers = 2 * _octaves + 1;

      _bufferSizeInFloats = kFloatsPerDSPVector * channels;

      // each octave uses one filter for each channel.
      _filters.resize(_octaves * channels);

      // get all buffers as a single contiguous array of floats.
      _buffers.resize(_bufferSizeInFloats * _numBuffers);
    }
  }
  ~Downsampler() = default;

  // write a vector of samples to the filter chain, run filters, and return
  // true if there is a new vector of output to read (every 2^octaves writes)
  template <size_t CHANNELS>
  bool write(DSPVectorArray<CHANNELS> v)
  {
    if (_octaves)
    {
      // write input to one of first two buffers
      const float* pSrc = v.getConstBuffer();
      float* pDest = bufferPtr(_counter & 1, 0);
      std::copy(pSrc, pSrc + kFloatsPerDSPVector * CHANNELS, pDest);

      // look at the bits of the counter from lowest to highest.
      // there is one bit for each octave of downsampling.
      // each octave is run if its bit and all lesser bits are 1.
      uint32_t mask = 1;
      for (int h = 0; h < _octaves; ++h)
      {
        bool b0 = _counter & mask;
        if (!b0) break;
        mask <<= 1;
        bool b1 = _counter & mask;

        // downsample each channel of the buffer
        for (int c = 0; c < CHANNELS; ++c)
        {
          HalfBandFilter* f = &(_filters[h * CHANNELS + c]);
          DSPVector vSrc1(bufferPtr(h * 2, c));
          DSPVector vSrc2(bufferPtr(h * 2 + 1, c));
          DSPVector vDest = f->downsample(vSrc1, vSrc2);
          store(vDest, bufferPtr(h * 2 + 2 + b1, c));
        }
      }

      // advance and wrap counter. If it's back to 0, we have output
      uint32_t counterMask = (1 << _octaves) - 1;
      _counter = (_counter + 1) & counterMask;
      return (_counter == 0);
    }
    else
    {
      // write input to final buffer
      const float* pSrc = v.getConstBuffer();
      float* pDest = bufferPtr(_numBuffers - 1, 0);
      std::copy(pSrc, pSrc + kFloatsPerDSPVector * CHANNELS, pDest);
      return true;
    }
  }

  template <size_t CHANNELS>
  DSPVectorArray<CHANNELS> read()
  {
    return DSPVectorArray<CHANNELS>(bufferPtr(_numBuffers - 1, 0));
  }
};

// PLL: Phase Locked Loop for synching an output phasor to an input phasor at some ratio.

class PLL
{
  // phasor on [0. - 1.), changes at rate of input phasor * input ratio
  float _omega{0};
  float _x1{0};

 public:
  // negative phase signals unknown offset.
  void clear() { _omega = -1.f; }

  // function call takes 3 inputs:
  // x: the input phasor to follow
  // dydx: the ratio to the input at which to lock the output phasor
  // feedback: amount of feedback to apply in PLL loop.
  // 1.0/sampleRate is a good amount of feedback to start with.
  DSPVector operator()(DSPVector x, DSPVector dydx, DSPVector feedback)
  {
    DSPVector y;

    // if input phasor is inactive, reset and bail.
    // (inactive / active switch is only done every vector)
    if (x[0] < 0.f)
    {
      clear();
      y = DSPVector(-1.f);
    }
    else
    {
      // startup: if active but phase is unknown, jump to current phase.
      if (_omega == -1.f)
      {
        // estimate previous input sample
        _x1 = x[0] - (x[1] - x[0]);

        _omega = fmod(x[0] * dydx[0], 1.0f);
      }

      DSPVector dxdy = divideApprox(DSPVector(1.0f), dydx);

      // run the PLL, correcting the output phasor to the input phasor and ratio.

      for (int n = 0; n < kFloatsPerDSPVector; ++n)
      {
        // TODO try using SIMD for differentiator object
        float px = x[n];
        float dxdt = px - _x1;
        if (dxdt < 0.f) dxdt += 1.f;
        _x1 = px;

        float dydt = dxdt * dydx[n];

        // get error term at each sample by comparing output to scaled input
        // or scaled input to output depending on ratio.
        float error;
        if (dydx[n] >= 1.f)
        {
          error = _omega - fmod(px * dydx[n], 1.0f);
        }
        else
        {
          error = fmod(_omega * dxdy[n], 1.0f) - px;
        }

        // send error towards closest sync
        error = roundf(error) - error;

        // feedback = negative error * time constant
        dydt += feedback[n] * error;

        // don't ever run clock backwards.
        dydt = ml::max(dydt, 0.f);

        // wrap phasor
        // TODO try using SIMD for fmod(x, 1.0), test
        _omega = fmod(_omega + dydt, 1.0f);

        y[n] = _omega;
      }
    }
    return y;
  }
};

}  // namespace ml
