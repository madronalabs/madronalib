// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
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

#include "MLDSPOps.h"
#include "MLDSPScalarMath.h"
#include <cmath>

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
// Thanks to Andrew Simper [www.cytomic.com] for sharing his work over the years.

struct Lopass
{
  enum coeffNames {g0, g1, g2, nCoeffs};
  enum paramNames {omega, k, nParams};
  enum stateNames {ic1eq, ic2eq, nStateVars};
  
  typedef std::array<float, nCoeffs> Coeffs;
  typedef DSPVectorArray<nCoeffs> CoeffsVec;
  typedef std::array<float, nParams> Params;
  typedef std::array<float, nStateVars> State;
  
  Coeffs coeffs{};
  State state{};

  inline void clear() { state.fill(0.f); }
  
  WIP
  // WIP
  // template<typename T> T lopassCalc(T x)
  // this will do x1 (float) and x4 (SIMD) versions
  // further templates will use x4 version to create Banks of COLS, ROWS
  
  // get internal coefficients for a given omega and k.
  // omega: the frequency divided by the sample rate.
  // k: 1/Q, where k=0 is maximum resonance.
  static Coeffs makeCoeffs(float omega, float k)
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

  // TODO const DSPVectorArray< nParams > params
  static CoeffsVec makeCoeffsVec(DSPVector omega, DSPVector k)
  {
    CoeffsVec vy;
    // TODO SIMD
    omega = min(omega, DSPVector(0.5f));
    k = max(k, DSPVector(0.01f));

    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float piOmega = kPi * omega[n];
      float s1 = sinf(piOmega);
      float s2 = sinf(2.0f * piOmega);
      float nrm = 1.0f / (2.f + k[n] * s2);
      vy.row(g0)[n] = s2 * nrm;
      vy.row(g1)[n] = (-2.f * s1 * s1 - k[n] * s2) * nrm;
      vy.row(g2)[n] = (2.0f * s1 * s1) * nrm;
    }
    return vy;
  }

  // filter the input vector vx with the stored coefficients.
  DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float t0 = v0 - state[ic2eq];
      float t1 = coeffs[g0] * t0 + coeffs[g1] * state[ic1eq];
      float t2 = coeffs[g2] * t0 + coeffs[g0] * state[ic1eq];
      float v2 = t2 + state[ic2eq];
      state[ic1eq] += 2.0f * t1;
      state[ic2eq] += 2.0f * t2;
      vy[n] = v2;
    }
    return vy;
  }

  // filter the input vector vx with the coefficients generated from parameters omega and k.
  // TODO const DSPVectorArray< nCoeffs >
  DSPVector operator()(const DSPVector vx, const DSPVector omega, const DSPVector k)
  {
    DSPVector vy;
    auto vc = makeCoeffsVec(omega, k);
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float t0 = v0 - state[ic2eq];
      float t1 = vc.constRow(g0)[n] * t0 + vc.constRow(g1)[n] * state[ic1eq];
      float t2 = vc.constRow(g2)[n] * t0 + vc.constRow(g0)[n] * state[ic1eq];
      float v2 = t2 + state[ic2eq];
      state[ic1eq] += 2.0f * t1;
      state[ic2eq] += 2.0f * t2;
      vy[n] = v2;
    }
    return vy;
  }
};

class Hipass
{
  struct Coeffs
  {
    float g0, g1, g2, k;
  };

  float ic1eq{0};
  float ic2eq{0};

 public:
  Coeffs coeffs{0};

  static Coeffs makeCoeffs(float omega, float k)
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
      float t1 = coeffs.g0 * t0 + coeffs.g1 * ic1eq;
      float t2 = coeffs.g2 * t0 + coeffs.g0 * ic1eq;
      float v1 = t1 + ic1eq;
      float v2 = t2 + ic2eq;
      ic1eq += 2.0f * t1;
      ic2eq += 2.0f * t2;
      vy[n] = v0 - coeffs.k * v1 - v2;
    }
    return vy;
  }
};

class Bandpass
{
  struct Coeffs
  {
    float g0, g1, g2;
  };

  float ic1eq{0};
  float ic2eq{0};

 public:
  Coeffs coeffs{0};

  static Coeffs makeCoeffs(float omega, float k)
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
      float t1 = coeffs.g0 * t0 + coeffs.g1 * ic1eq;
      float t2 = coeffs.g2 * t0 + coeffs.g0 * ic1eq;
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
    nCoeffs
  };
  typedef std::array<float, nCoeffs> Coeffs;
  typedef DSPVectorArray<nCoeffs> VCoeffs;

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
  Coeffs coeffs{};

  static Coeffs makeCoeffs(params p)
  {
    Coeffs r;
    float piOmega = kPi * p[omega];
    float g = tanf(piOmega) / sqrtf(p[A]);
    r[a1] = 1.f / (1.f + g * (g + p[k]));
    r[a2] = g * r[a1];
    r[a3] = g * r[a2];
    r[m1] = p[k] * (p[A] - 1.f);
    r[m2] = (p[A] * p[A] - 1.f);
    return r;
  }

  static VCoeffs vcoeffs(const params p0, const params p1)
  {
    return interpolateCoeffsLinear(makeCoeffs(p0), makeCoeffs(p1));
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float v3 = v0 - ic2eq;
      float v1 = coeffs[a1] * ic1eq + coeffs[a2] * v3;
      float v2 = ic2eq + coeffs[a2] * ic1eq + coeffs[a3] * v3;
      ic1eq = 2 * v1 - ic1eq;
      ic2eq = 2 * v2 - ic2eq;
      vy[n] = v0 + coeffs[m1] * v1 + coeffs[m2] * v2;
    }
    return vy;
  }

  inline DSPVector operator()(const DSPVector vx, const VCoeffs vc)
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
  enum coeffNames
  {
    a1,
    a2,
    a3,
    m0,
    m1,
    m2,
    nCoeffs
  };
  typedef std::array<float, nCoeffs> Coeffs;
  typedef DSPVectorArray<nCoeffs> VCoeffs;

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
  Coeffs coeffs{};

  static Coeffs makeCoeffs(params p)
  {
    Coeffs r;
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

  static VCoeffs vcoeffs(const params p0, const params p1)
  {
    return interpolateCoeffsLinear(makeCoeffs(p0), makeCoeffs(p1));
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      float v0 = vx[n];
      float v3 = v0 - ic2eq;
      float v1 = coeffs[a1] * ic1eq + coeffs[a2] * v3;
      float v2 = ic2eq + coeffs[a2] * ic1eq + coeffs[a3] * v3;
      ic1eq = 2 * v1 - ic1eq;
      ic2eq = 2 * v2 - ic2eq;
      vy[n] = coeffs[m0] * v0 + coeffs[m1] * v1 + coeffs[m2] * v2;
    }
    return vy;
  }

  inline DSPVector operator()(const DSPVector vx, const VCoeffs vc)
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
  struct Coeffs
  {
    float a1, a2, a3, m1;
  };

  float ic1eq{0};
  float ic2eq{0};

 public:
  Coeffs coeffs{0};

  static Coeffs makeCoeffs(float omega, float k, float A)
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
      float v1 = coeffs.a1 * ic1eq + coeffs.a2 * v3;
      float v2 = ic2eq + coeffs.a2 * ic1eq + coeffs.a3 * v3;
      ic1eq = 2 * v1 - ic1eq;
      ic2eq = 2 * v2 - ic2eq;
      vy[n] = v0 + coeffs.m1 * v1;
    }
    return vy;
  }
};

// A one pole filter. see https://ccrma.stanford.edu/~jos/fp/One_Pole.html

struct OnePole
{
  struct Coeffs
  {
    float a0, b1;
  };

  float y1{0};

 public:
  Coeffs coeffs{0};

  static Coeffs makeCoeffs(float omega)
  {
    float x = expf(-omega * kTwoPi);
    return {1.f - x, x};
  }

  static Coeffs passthru() { return {1.f, 0.f}; }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      y1 = coeffs.a0 * vx[n] + coeffs.b1 * y1;
      vy[n] = y1;
    }
    return vy;
  }

  // jump to the new output value f without slewing there.
  void reset(float f) { y1 = f; }

  void clear() { y1 = 0.f; }
};

// A one-pole, one-zero filter to attenuate DC.
// Works well, but beware of its effects on bass sounds.
// A "cutoff" of around 2kHz (omega = 0.045 at sr=44100) is a
// good starting point. see https://ccrma.stanford.edu/~jos/fp/DC_Blocker.html
// for more.

class DCBlocker
{
  typedef float Coeffs;
  float x1{0};
  float y1{0};

 public:
  Coeffs coeffs{0.045f};

  static Coeffs makeCoeffs(float omega) { return cosf(omega); }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      const float x0 = vx[n];
      const float y0 = x0 - x1 + coeffs * y1;
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
  float _x1{0};

 public:
  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    vy[0] = vx[0] - _x1;

    // TODO SIMD
    for (int n = 1; n < kFloatsPerDSPVector; ++n)
    {
      vy[n] = vx[n] - vx[n - 1];
    }
    _x1 = vx[kFloatsPerDSPVector - 1];
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
  struct Coeffs
  {
    float a0, b1;
  };

  float y1{0};
  int peakHoldCounter{0};

 public:
  Coeffs coeffs{0};
  int peakHoldSamples{44100};

  static Coeffs makeCoeffs(float omega)
  {
    float x = expf(-omega * kTwoPi);
    return {1.f - x, x};
  }

  static Coeffs passthru() { return {1.f, 0.f}; }

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
          y1 = coeffs.a0 * vxSquared[n] + coeffs.b1 * y1;
        }
      }
      vy[n] = y1;
    }

    if (peakHoldCounter > 0)
    {
      peakHoldCounter -= kFloatsPerDSPVector;
    }

    // use sqrt approximation. Return 0 for inputs near 0.
    return select(sqrtApprox(vy), DSPVector{0.f}, greaterThan(vy, DSPVector{float(1e-20)}));
  }
};

// filtered RMS

class RMS
{
  struct Coeffs
  {
    float a0, b1;
  };

  float y1{0};

 public:
  Coeffs coeffs{0};

  static Coeffs makeCoeffs(float omega)
  {
    float x = expf(-omega * kTwoPi);
    return {1.f - x, x};
  }

  static Coeffs passthru() { return {1.f, 0.f}; }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector vy;
    DSPVector vxSquared = vx * vx;

    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      y1 = coeffs.a0 * vxSquared[n] + coeffs.b1 * y1;
      vy[n] = y1;
    }

    // use sqrt approximation. Return 0 for inputs near 0.
    return select(sqrtApprox(vy), DSPVector{0.f}, greaterThan(vy, DSPVector{float(1e-20)}));
  }
};

// ADSR envelope triggered and scaled by a single gate + amp signal.

struct ADSR
{
  static constexpr float bias = 0.1f;
  static constexpr float minSegmentTime = 0.0002f;

  struct Coeffs
  {
    float ka, kd, s, kr;
  };

  enum segments
  {
    A = 0,
    D = 1,
    S = 2,
    R = 3,
    off = 4
  };

  static Coeffs calcCoeffs(float a, float d, float s, float r, float sr)
  {
    const float invSr = 1.0f / sr;
    const float ka = kTwoPi * invSr / ml::max(a, minSegmentTime);
    const float kd = kTwoPi * invSr / ml::max(d, minSegmentTime);
    const float kr = kTwoPi * invSr / ml::max(r, minSegmentTime);
    return {ka, kd, s, kr};
  }

  Coeffs coeffs{0};

  float y{0};          // current output
  float y1{0};         // previous output
  float x1{0};         // previous input
  float threshold{0};  // actual value to stop on
  float target{0};     // input to filter: value with bias added so we end in a finite time
  float k{0};          // IIR filter coefficient
  float amp{0};
  int segment{off};

  void clear() { segment = off; }

  inline float processSample(float x)
  {
    if ((segment == off) && (x == 0.f)) return 0.f;

    bool crossedThresh = ((y1 > threshold) != (y > threshold));
    bool recalc{false};

    // crossing threshold advances to next envelope segment
    if (crossedThresh && (segment < off))
    {
      segment++;  // << ?
      recalc = true;
    }

    bool trigOn = (x1 == 0.f) && (x > 0.f);
    bool trigOff = (x1 > 0.f) && (x == 0.f);

    // TODO use some bit twiddling
    if (trigOn)
    {
      segment = A;
      amp = x;
      recalc = true;
    }
    else if (trigOff)
    {
      segment = R;
      recalc = true;
    }

    if (recalc)
    {
      float startEnv, endEnv;
      switch (segment)
      {
        case A:
        {
          startEnv = 0.f;
          endEnv = 1.f;
          k = coeffs.ka;
          break;
        }
        case D:
        {
          startEnv = 1.f;
          endEnv = coeffs.s;
          k = coeffs.kd;
          break;
        }
        case S:
        {
          startEnv = coeffs.s;
          endEnv = coeffs.s;
          k = 0.f;
          y1 = coeffs.s;
          y = coeffs.s;
          break;
        }
        case R:
        {
          startEnv = coeffs.s;
          endEnv = 0.f;
          k = coeffs.kr;
          break;
        }
        case off:
        {
          startEnv = 0.f;
          endEnv = 0.f;
          k = 0.f;
          y1 = 0.f;
          y = 0.f;
          break;
        }
      }

      float segmentBias = (endEnv - startEnv) * bias;
      threshold = endEnv;
      target = endEnv + segmentBias;
    }

    // history and IIR filter
    x1 = x;
    y1 = y;
    y = y + k * (target - y);

    // scale by amp
    return y * amp;
  }

  inline DSPVector operator()(const DSPVector vx)
  {
    DSPVector r;
    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      r[i] = processSample(vx[i]);
    }
    return r;
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
    setMaxDelayInSamples(static_cast<float>(d));
    setDelayInSamples(d);
  }
  ~IntegerDelay() = default;

  // for efficiency, no bounds checking is done. Because mLengthMask is used to
  // constrain all reads, bad values here may make bad sounds (buffer wraps) but
  // will not attempt to read from outside the buffer.
  inline void setDelayInSamples(int d) { mIntDelayInSamples = d; }

  void setMaxDelayInSamples(float d)
  {
    int dMax = static_cast<int>(floorf(d));
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
  typedef float Coeffs;
  Coeffs coeffs{0.f};

  Allpass1(float a) : coeffs(a) {}
  ~Allpass1() {}

  inline void clear()
  {
    x1 = 0.f;
    y1 = 0.f;
  }

  // get allpass coefficient from a delay fraction d.
  // to minimize modulation noise, d should be in the range [0.618 - 1.618].
  static float makeCoeffs(float d)
  {
    // return 2nd order approx around 1 to (1.f - d) / (1.f + d)
    float xm1 = (d - 1.f);
    return -0.53f * xm1 + 0.24f * xm1 * xm1;
  }

  inline float processSample(const float x)
  {
    // one-multiply form. see
    // https://ccrma.stanford.edu/~jos/pasp/One_Multiply_Scattering_Junctions.html
    float y = x1 + (x - y1) * coeffs;
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
  Allpass1 mAllpassSection{0.f};
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
    int delayInt = static_cast<int>(fDelayInt);
    float delayFrac = d - fDelayInt;

    // constrain D to [0.618 - 1.618] if possible
    if ((delayFrac < 0.618f) && (delayInt > 0))
    {
      delayFrac += 1.f;
      delayInt -= 1;
    }
    mIntegerDelay.setDelayInSamples(delayInt);
    mAllpassSection.coeffs = Allpass1::makeCoeffs(delayFrac);
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
      mFilters[n].coeffs = ml::OnePole::makeCoeffs(omegas[n]);
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

  void clear()
  {
    apa0.clear();
    apa1.clear();
    apb0.clear();
    apb1.clear();
    b1 = 0;
  }

 private:
  // order=4, rejection=70dB, transition band=0.1.
  Allpass1 apa0{0.07986642623635751f}, apa1{0.5453536510711322f}, apb0{0.28382934487410993f},
      apb1{0.8344118914807379f};
  float b1{0};
};

// Downsampler
// a cascade of half band filters, one for each octave.
// TODO this is complicated. replace with single-channel version then use Bank<Downsampler> for
// multiple channels?
class Downsampler
{
  std::vector<HalfBandFilter> _filters;
  std::vector<float> _buffers;
  int _octaves;
  int _numBuffers;
  uint32_t _counter{0};

  float* bufferPtr(int idx) { return _buffers.data() + idx * kFloatsPerDSPVector; }

 public:
  Downsampler(int octavesDown) : _octaves(octavesDown)
  {
    if (_octaves)
    {
      // one pair of buffers for each octave plus one output buffer.
      _numBuffers = 2 * _octaves + 1;

      // each octave uses one filter.
      _filters.resize(_octaves);

      // get all buffers as a single contiguous array of floats.
      _buffers.resize(kFloatsPerDSPVector * _numBuffers);

      clear();
    }
  }
  ~Downsampler() = default;

  // write a vector of samples to the filter chain, run filters, and return
  // true if there is a new vector of output to read (every 2^octaves writes)
  bool write(DSPVector v)
  {
    if (_octaves)
    {
      // write input to one of first two buffers
      const float* pSrc = v.getConstBuffer();
      float* pDest = bufferPtr(_counter & 1);
      std::copy(pSrc, pSrc + kFloatsPerDSPVector, pDest);

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

        // run filter
        HalfBandFilter* f = &(_filters[h]);
        DSPVector vSrc1(bufferPtr(h * 2));
        DSPVector vSrc2(bufferPtr(h * 2 + 1));
        DSPVector vDest = f->downsample(vSrc1, vSrc2);
        store(vDest, bufferPtr(h * 2 + 2 + b1));
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
      float* pDest = bufferPtr(_numBuffers - 1);
      std::copy(pSrc, pSrc + kFloatsPerDSPVector, pDest);
      return true;
    }
  }

  DSPVector read() { return DSPVector(bufferPtr(_numBuffers - 1)); }

  void clear()
  {
    for (auto& f : _filters)
    {
      f.clear();
    }
    std::fill(_buffers.begin(), _buffers.end(), 0.f);
    _counter = 0;
  }
};

struct Upsampler
{
  std::vector<HalfBandFilter> _filters;
  std::vector<float> _buffers;
  int _octaves;
  int _numBuffers;
  int readIdx_{0};

  float* bufferPtr(int idx) { return _buffers.data() + idx * kFloatsPerDSPVector; }

  Upsampler(int octavesUp) : _octaves(octavesUp)
  {
    if (_octaves)
    {
      _numBuffers = 1 << _octaves;
      size_t numFilters = _octaves;
      _filters.resize(numFilters);

      // get all buffers as a single contiguous array of floats.
      _buffers.resize(kFloatsPerDSPVector * _numBuffers);

      clear();
    }
  }
  ~Upsampler() = default;

  void write(DSPVector x)
  {
    // write to last vector in buffer
    store(x, bufferPtr(_numBuffers - 1));

    // for each octave of upsampling, upsample blocks to twice as many, in place, ending at buffers
    // end
    for (int j = 0; j < _octaves; ++j)
    {
      int sourceBufs = 1 << j;
      int destBufs = sourceBufs << 1;
      int srcStart = _numBuffers - sourceBufs;
      int destStart = _numBuffers - destBufs;

      for (int i = 0; i < sourceBufs; ++i)
      {
        DSPVector src, dest1, dest2;
        load(src, bufferPtr(srcStart + i));
        dest1 = _filters[j].upsampleFirstHalf(src);
        dest2 = _filters[j].upsampleSecondHalf(src);
        store(dest1, bufferPtr(destStart + (i * 2)));
        store(dest2, bufferPtr(destStart + (i * 2) + 1));
      }
    }
    readIdx_ = 0;
  }

  // after a write, 1 << octaves reads are available.
  DSPVector read()
  {
    DSPVector result;
    load(result, bufferPtr(readIdx_++));
    return result;
  }

  void clear()
  {
    for (auto& f : _filters)
    {
      f.clear();
    }
    std::fill(_buffers.begin(), _buffers.end(), 0.f);
    readIdx_ = 0;
  }
};

// From an input clock phasor and an output/input frequency ratio,
// produce an output clock at the given ratio that is phase-synched with the input.
//
class TempoLock
{
  // phasor on [0. - 1.), changes at rate of input phasor * input ratio
  float _omega{-1.f};  // current output phase
  float _x1v{0};       // input one vector ago

 public:
  // phase of -1 means we are stopped.
  void clear() { _omega = -1.0f; }

  // function call takes 3 inputs:
  // x: the input phasor to follow
  // dydx: the ratio to the input at which to lock the output phasor
  // isr: inverse of sample rate
  DSPVector operator()(DSPVector x, float dydx, float isr)
  {
    DSPVector y;
    float x0 = x[0];
    float dxdt{0.f};
    float dydt{0.f};

    // if input phasor is inactive, reset and output 0.
    // we check against -1 because a running input phasor may be slightly
    // less than zero.
    if (x0 == -1.0f)
    {
      clear();
      y = DSPVector(0.f);
    }
    else
    {
      // get dxdt and dydt from input and ratio
      if (_omega > -1.f)
      {
        // if we are already running: get average input slope every vector
        float dx = x0 - _x1v;
        if (dx < 0.f) dx += 1.f;
        dxdt = dx / kFloatsPerDSPVector;
        dydt = dxdt * dydx;
        _x1v = x0;
      }
      else
      {
        // on startup: we are active but phase is unknown, so jump to
        // current phase based on input.
        dxdt = x[1] - x0;
        dydt = dxdt * dydx;
        _x1v = x0 - dxdt * kFloatsPerDSPVector;
        _omega = fmod(x0 * dydx, 1.0f);
      }

      // if the ratio of its reciprocal is close to an integer, lock to input phase
      bool lock{false};
      constexpr float lockDist = 0.001f;
      if (fabs(dydx - roundf(dydx)) < lockDist) lock = true;
      float rdydx = 1.0f / dydx;
      if (fabs(rdydx - roundf(rdydx)) < lockDist) lock = true;

      if (lock)
      {
        // get error term at each vector by comparing output to scaled input
        // or scaled input to output depending on ratio.
        float ref, refWrap, error;
        if (dydx >= 1.f)
        {
          ref = x0 * dydx;
          refWrap = ref - floorf(ref);
          error = _omega - refWrap;
        }
        else
        {
          ref = _omega / dydx;
          refWrap = ref - floorf(ref);
          error = refWrap - x0;
        }

        // get error difference from closest sync target
        float errorDiff = roundf(error) - error;

        // add error correction term to dydt. Note that this is only added to the current vector.
        // this is different from a traditional PLL, which would need a filter in the feedback loop.
        //
        // this addition tweaks the slope to reach the target value in 1/4 second. However as
        // the target gets closer the slope is less, resulting in an exponentially slowing approach.
        float correction = errorDiff * isr * 4.0f;

        // don't allow going under 0.5x or over 2x speed
        correction = clamp(correction, -dydt * 0.5f, dydt * 1.0f);
        dydt += correction;
      }

      // make output vector with sample-accurate wrap
      for (int i = 0; i < kFloatsPerDSPVector; ++i)
      {
        y[i] = _omega;
        _omega += dydt;
        if (_omega > 1.0f) _omega -= 1.0f;
      }
    }
    return y;
  }
};

}  // namespace ml
