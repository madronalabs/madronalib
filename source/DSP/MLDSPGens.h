// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// DSP generators: functor objects implementing an inline DSPVector operator()
// in order to make time-varying signals. Generators all have some state, for
// example the frequency of an oscillator or the seed in a noise generator.
// Otherwise they would be DSPOps.
//
// These objects are for building fixed DSP graphs in a functional style. The
// compiler should have many opportunities to optimize these graphs. For dynamic
// graphs changeable at runtime, see MLProcs. In general MLProcs will be written
// using DSPGens, DSPOps, DSPFilters.

#pragma once

#include "MLDSPOps.h"

namespace ml
{
// generate a single-sample tick every n samples.

class TickGen
{
 public:
  TickGen(int p) : mCounter(p), mPeriod(p) {}
  ~TickGen() {}

  inline void setPeriod(int p) { mPeriod = p; }

  inline DSPVector operator()()
  {
    DSPVector vy;
    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      float fy = 0;
      if (++mCounter >= mPeriod)
      {
        mCounter = 0;
        fy = 1;
      }
      vy[i] = fy;
    }
    return vy;
  }
  int mCounter;
  int mPeriod;
};

// antialiased ImpulseGen TODO

class NoiseGen
{
 public:
  NoiseGen() : mSeed(0) {}
  ~NoiseGen() {}

  inline void step() { mSeed = mSeed * 0x0019660D + 0x3C6EF35F; }

  inline uint32_t getIntSample()
  {
    step();
    return mSeed;
  }

  inline float getSample()
  {
    step();
    uint32_t temp = ((mSeed >> 9) & 0x007FFFFF) | 0x3F800000;
    return (*reinterpret_cast<float*>(&temp)) * 2.f - 3.f;
  }

  // TODO SIMD
  inline DSPVector operator()()
  {
    DSPVector y;
    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      step();
      uint32_t temp = ((mSeed >> 9) & 0x007FFFFF) | 0x3F800000;
      y[i] = (*reinterpret_cast<float*>(&temp)) * 2.f - 3.f;
    }
    return y;
  }

  void reset() { mSeed = 0; }

 private:
  uint32_t mSeed = 0;
};

// if up at MLProc level, all outputs have fixed sizes, procs like sine16,
// sine64, sine256, sine1024 can be used this is probably not the worst thing
// what is penalty of dynamic sizing?
//
// proc can have a "size" switch on it that creates different gens internally.
// but, resizing graph dynamically is complex. outputs auto-sum to smaller
// inputs?

/*

 0 operands (generators):
 sineOsc
 TriOsc
 PhaseOsc

 ramp generator
 quadratic generator

 banks:
 ----
 sinebank
 phasebank
 SVFbank
 biquadbank
 delaybank
 hooooold on...

 a bank of raised cos generators can be for a granulator or shepard tone
 generator

*/

// super slow + accurate sine generator for testing

class TestSineGen
{
  float mOmega{0};

 public:
  void clear() { mOmega = 0; }

  DSPVector operator()(const DSPVector freq)
  {
    DSPVector vy;

    for (int i = 0; i < kFloatsPerDSPVector; ++i)
    {
      float step = ml::kTwoPi * freq[i];
      mOmega += step;
      if (mOmega > ml::kTwoPi) mOmega -= ml::kTwoPi;
      vy[i] = sinf(mOmega);
    }
    return vy;
  }
};

// PhasorGen is a naive (not antialiased) sawtooth generator.
// These can be useful for a few things, like controlling wavetable playback.
// it takes one input vector: the radial frequency in cycles per sample (f/sr).
// it outputs a phasor with range from 0--1.
class PhasorGen
{
  int32_t mOmega32{0};

 public:
  void clear(int32_t omega = 0) { mOmega32 = omega; }

  DSPVector operator()(const DSPVector cyclesPerSample)
  {
    constexpr float range(1.0f);
    constexpr float offset(0.5f);
    constexpr float stepsPerCycle(const_math::pow(2., 32.));
    DSPVector outputScaleV(range / stepsPerCycle);

    // calculate int steps per sample
    DSPVector stepsPerSampleV = cyclesPerSample * DSPVector(stepsPerCycle);
    DSPVectorInt intStepsPerSampleV = roundFloatToInt(stepsPerSampleV);

    // accumulate 32-bit phase with wrap
    DSPVectorInt omega32V;
    for (int n = 0; n < kIntsPerDSPVector; ++n)
    {
      mOmega32 += intStepsPerSampleV[n];
      omega32V[n] = mOmega32;
    }

    // convert counter to float output range
    DSPVector omegaV = intToFloat(omega32V) * outputScaleV + DSPVector(offset);
    return omegaV;
  }
};

// bandlimited step function for reducing aliasing.
static DSPVector polyBLEP(const DSPVector phase, const DSPVector freq)
{
  DSPVector blep;

  for (int n = 0; n < kFloatsPerDSPVector; ++n)
  {
    // could possibly differentiate to get dt instead of passing it in.
    // but that would require state.
    float t = phase[n];
    float dt = freq[n];

    // TODO try SIMD optimization
    float c{0.f};
    if (t < dt)
    {
      t = t / dt;
      c = t + t - t * t - 1.0f;
    }
    else if (t > 1.0f - dt)
    {
      t = (t - 1.0) / dt;
      c = t * t + t + t + 1.0f;
    }

    blep[n] = c;
  }
  return blep;
}

// input: phasor on (0, 1)
// output: sine aproximation using Taylor series on range(-1, 1). There is distortion in odd
// harmonics only, with the 3rd harmonic at about -40dB.
inline DSPVector phasorToSine(DSPVector phasorV)
{
  constexpr float sqrt2(const_math::sqrt(2.0f));
  constexpr float domain(sqrt2 * 4.f);
  DSPVector domainScaleV(domain);
  DSPVector domainOffsetV(-sqrt2);
  constexpr float range(sqrt2 - sqrt2 * sqrt2 * sqrt2 / 6.f);
  DSPVector scaleV(1.0f / range);
  DSPVector flipOffsetV(sqrt2 * 2.f);
  DSPVector zeroV(0.f);
  DSPVector oneV(1.f);
  DSPVector oneSixthV(1.0f / 6.f);

  // scale and offset input phasor on (0, 1) to sine approx domain (-sqrt(2), 3*sqrt(2))
  DSPVector omegaV = phasorV * (domainScaleV) + (domainOffsetV);

  // reverse upper half of phasor to get triangle
  // equivalent to: if (phasor > 0) x = flipOffset - fOmega; else x = fOmega;
  DSPVector triangleV = select(flipOffsetV - omegaV, omegaV, greaterThan(omegaV, DSPVector(sqrt2)));

  // convert triangle to sine approx.
  return scaleV * triangleV * (oneV - triangleV * triangleV * oneSixthV);
}

// input: phasor on (0, 1), normalized freq, pulse width
// output: antialiased pulse
inline DSPVector phasorToPulse(DSPVector omegaV, DSPVector freqV, DSPVector pulseWidthV)
{
  // get pulse selector mask
  DSPVectorInt maskV = greaterThan(omegaV, pulseWidthV);

  // select -1 or 1 (could be a multiply instead?)
  DSPVector pulseV = select(DSPVector(-1.f), DSPVector(1.f), maskV);

  // add blep for up-going transition
  pulseV += polyBLEP(omegaV, freqV);

  // subtract blep for down-going transition
  DSPVector omegaVDown = fractionalPart(omegaV - pulseWidthV + DSPVector(1.0f));
  pulseV -= polyBLEP(omegaVDown, freqV);

  return pulseV;
}

// input: phasor on (0, 1), normalized freq
// output: antialiased saw on (-1, 1)
inline DSPVector phasorToSaw(DSPVector omegaV, DSPVector freqV)
{
  // scale phasor to saw range (-1, 1)
  DSPVector sawV = omegaV * DSPVector(2.f) - DSPVector(1.f);

  // subtract BLEP from saw to smooth down-going transition
  return sawV - polyBLEP(omegaV, freqV);
}

// these antialiased waveform generators use a PhasorGen and the functions above.

class SineGen
{
  static constexpr int32_t kZeroPhase = -(2 << 29);
  PhasorGen _phasor;

 public:
  void clear() { _phasor.clear(kZeroPhase); }
  DSPVector operator()(const DSPVector freq) { return phasorToSine(_phasor(freq)); }
};

class PulseGen
{
  PhasorGen _phasor;

 public:
  void clear() { _phasor.clear(0); }
  DSPVector operator()(const DSPVector freq, const DSPVector width)
  {
    return phasorToPulse(_phasor(freq), freq, width);
  }
};

class SawGen
{
  PhasorGen _phasor;

 public:
  void clear() { _phasor.clear(0); }
  DSPVector operator()(const DSPVector freq) { return phasorToSaw(_phasor(freq), freq); }
};

// ----------------------------------------------------------------
// LinearGlide

// convert a scalar float input into a DSPVector with linear slew.
// to allow optimization, glide time is quantized to DSPVectors.
// Note that a onepole or other IIR filter is not used because we must reach
// the actual value in a finite time.

constexpr float unityRampFn(int i) { return (i + 1) / static_cast<float>(kFloatsPerDSPVector); }
ConstDSPVector kUnityRampVec{unityRampFn};

class LinearGlide
{
  DSPVector mCurrVec{0.f};
  DSPVector mStepVec{0.f};
  float mTargetValue{0};
  float mDyPerVector{1.f / 32};
  int mVectorsPerGlide{32};
  int mVectorsRemaining{0};

 public:
  void setGlideTimeInSamples(float t)
  {
    mVectorsPerGlide = t / kFloatsPerDSPVector;
    if (mVectorsPerGlide < 1) mVectorsPerGlide = 1;
    mDyPerVector = 1.0f / (mVectorsPerGlide + 0.f);
  }

  // set the current value to the given value immediately, without gliding
  void setValue(float f)
  {
    mTargetValue = f;
    mVectorsRemaining = 0;
  }

  DSPVector operator()(float f)
  {
    // set target value if different from current value.
    // const float currentValue = mCurrVec[kFloatsPerDSPVector - 1];
    if (f != mTargetValue)
    {
      mTargetValue = f;

      // start counter
      mVectorsRemaining = mVectorsPerGlide;
    }

    // process glide
    if (mVectorsRemaining == 0)
    {
      // end glide: write target value to output vector
      mCurrVec = DSPVector(mTargetValue);
      mStepVec = DSPVector(0.f);
      mVectorsRemaining--;
    }
    else if (mVectorsRemaining == mVectorsPerGlide)
    {
      // start glide: get change in output value per vector
      float currentValue = mCurrVec[kFloatsPerDSPVector - 1];
      float dydv = (mTargetValue - currentValue) * mDyPerVector;

      // get constant step vector
      mStepVec = DSPVector(dydv);

      // setup current vector with first interpolation ramp.
      mCurrVec = DSPVector(currentValue) + kUnityRampVec * DSPVector(dydv);

      mVectorsRemaining--;
    }
    else
    {
      // continue glide
      // Note that repeated adding will create some error in target value.
      // Because we return the target value explicity when we are done, this
      // won't be a problem in reasonably short glides.
      mCurrVec += mStepVec;
      mVectorsRemaining--;
    }

    return mCurrVec;
  }
};

// GenBanks can go here

/*
 banks:
----
sinebank
noisebank
oscbank

 template<int VECTORS>
 class SineBank
 {
 // float will be promoted to Matrix of size 1 for single argument
 SineBank<VECTORS>(Matrix f) { setFrequency(f); clear(); }
 ~SineBank<VECTORS>() {}

 inline DSPVectorArray<VECTORS> operator()()
 {
 DSPVectorArray<VECTORS> y;
 for(int j=0; j<VECTORS; ++j)
 {

 }
 return y;
 }

 private:
 int32_t mOmega32, mStep32;
 float mInvSrDomain;
 };

 typedef SineBank<1> Sine;
 */

//
}  // namespace ml
