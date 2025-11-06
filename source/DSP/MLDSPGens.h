// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
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

#include "MLDSPFunctional.h"
#include "MLDSPOps.h"
#include "MLDSPUtils.h"

namespace ml
{
// generate a single-sample tick, repeating at a frequency given by the input.
class TickGen
{
  float mOmega{0};

 public:
  inline DSPVector operator()(const DSPVector cyclesPerSample)
  {
    // calculate counter delta per sample
    DSPVector stepsPerSampleV = cyclesPerSample;

    // accumulate phase and wrap to generate ticks
    DSPVector vy{0.f};
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      mOmega += stepsPerSampleV[n];
      if (mOmega > 1.0f)
      {
        mOmega -= 1.0f;
        vy[n] = 1.0f;
      }
    }
    return vy;
  }
};

// generate an antialiased impulse, repeating at a frequency given by the input.
// limitations to fix:
//     frequency can't be higher than sr / table size.
//     table output is only positioned to the nearest sample.
class ImpulseGen
{
  // pick odd table size to get sample-centered sinc and window
  static constexpr int kTableSize{17};
  DSPVector _table;
  static_assert(kTableSize < kFloatsPerDSPVector,
                "ImpulseGen: table size must be < the DSP vector size.");

  int _outputCounter{0};
  float _omega{0.f};

 public:
  ImpulseGen()
  {
    // make windowed sinc table
    DSPVector windowVec;
    makeWindow(windowVec.getBuffer(), kTableSize, dspwindows::blackman);
    const float omega = 0.25f;
    auto sincFn{[&](int i)
                {
                  float pi_x = ml::kTwoPi * omega * i;
                  return (i == 0) ? 1.f : sinf(pi_x) / pi_x;
                }};
    DSPVector sincVec = map(sincFn, columnIndexInt() - DSPVectorInt((kTableSize - 1) / 2));
    _table = normalize(sincVec * windowVec);
  }
  ~ImpulseGen() {}

  inline DSPVector operator()(const DSPVector cyclesPerSample)
  {
    // accumulate phase and wrap to generate ticks
    DSPVector vy{0.f};
    for (int n = 0; n < kFloatsPerDSPVector; ++n)
    {
      _omega += cyclesPerSample[n];
      if (_omega > 1.0f)
      {
        _omega -= 1.0f;

        // start an output impulse
        _outputCounter = 0;
      }

      if (_outputCounter < kTableSize)
      {
        vy[n] = _table[_outputCounter];
        _outputCounter++;
      }
    }
    return vy;
  }
};

// generate a random number from -1 to 1 every sample.
// NOTE: this will create more energy at higher sample rates!
// TODO make proper pink noise, white noise gens
class NoiseGen
{
 public:
  NoiseGen() : mSeed(0) {}
  ~NoiseGen() {}

  inline void step() { mSeed = mSeed * 0x0019660D + 0x3C6EF35F; }
  inline void setSeed(uint32_t x) { mSeed = x; }

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
  uint32_t mOmega32{0};

 public:
  void clear(uint32_t omega = 0) { mOmega32 = omega; }

  static constexpr float stepsPerCycle{static_cast<float>(const_math::pow(2., 32))};
  static constexpr float cyclesPerStep{1.f / stepsPerCycle};

  DSPVector operator()(const DSPVector cyclesPerSample)
  {
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
    return unsignedIntToFloat(omega32V) * DSPVector(cyclesPerStep);
  }

  float nextSample(const float cyclesPerSample)
  {
    // calculate int steps per sample
    float stepsPerSample = cyclesPerSample * stepsPerCycle;
    uint32_t intStepsPerSample = (uint32_t)roundf(stepsPerSample);

    // accumulate 32-bit phase with wrap
    mOmega32 += intStepsPerSample;

    // convert counter to float output range
    return mOmega32 * cyclesPerStep;
  }
};

// OneShotGen, when triggered, makes a single ramp from 0-1 then resets to 0. The speed
// of the ramp is a signal input, giving a ramp with the same speed as PhasorGen.
class OneShotGen
{
  static constexpr uint32_t start = 0;
  uint32_t mOmega32{start};
  uint32_t mGate{0};
  uint32_t mOmegaPrev{start};

 public:
  void trigger()
  {
    mOmega32 = mOmegaPrev = start;
    mGate = 1;
  }

  static constexpr float stepsPerCycle{static_cast<float>(const_math::pow(2., 32))};
  static constexpr float cyclesPerStep{1.f / stepsPerCycle};

  DSPVector operator()(const DSPVector cyclesPerSample)
  {
    // calculate int steps per sample
    DSPVector stepsPerSampleV = cyclesPerSample * DSPVector(stepsPerCycle);
    DSPVectorInt intStepsPerSampleV = roundFloatToInt(stepsPerSampleV);

    // accumulate 32-bit phase with wrap
    // we test for wrap at every sample to get a clean ending
    DSPVectorInt omega32V;
    for (int n = 0; n < kIntsPerDSPVector; ++n)
    {
      mOmega32 += intStepsPerSampleV[n] * mGate;
      if (mOmega32 < mOmegaPrev)
      {
        mGate = 0;
        mOmega32 = start;
      }
      omega32V[n] = mOmegaPrev = mOmega32;
    }
    // convert counter to float output range
    return unsignedIntToFloat(omega32V) * DSPVector(cyclesPerStep);
  }

  float nextSample(const float cyclesPerSample)
  {
    // calculate int steps per sample
    float stepsPerSample = cyclesPerSample * stepsPerCycle;
    uint32_t intStepsPerSample = (uint32_t)roundf(stepsPerSample);

    // accumulate 32-bit phase with wrap
    // we test for wrap at every sample to get a clean ending
    DSPVectorInt omega32V;

    mOmega32 += intStepsPerSample * mGate;
    if (mOmega32 < mOmegaPrev)
    {
      mGate = 0;
      mOmega32 = start;
    }
    mOmegaPrev = mOmega32;

    // convert counter to float output range
    return mOmega32 * cyclesPerStep;
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
      t = (t - 1.0f) / dt;
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
  constexpr float sqrt2(static_cast<float>(const_math::sqrt(2.0f)));
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
  DSPVectorInt maskV = greaterThanOrEqual(omegaV, pulseWidthV);

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
// Interpolator1

// linear interpolate over signal length to next value.

constexpr float unityRampFn(int i) { return (i + 1) / static_cast<float>(kFloatsPerDSPVector); }
ConstDSPVector kUnityRampVec{unityRampFn};

struct Interpolator1
{
  float currentValue{0};

  DSPVector operator()(float f)
  {
    float dydt = f - currentValue;
    DSPVector outputVec = DSPVector(currentValue) + kUnityRampVec * dydt;
    currentValue = f;
    return outputVec;
  }
};

// TODO needs test

// ----------------------------------------------------------------
// LinearGlide

// convert a scalar float input into a DSPVector with linear slew.
// to allow optimization, glide time is quantized to DSPVectors.

class LinearGlide
{
  DSPVector mCurrVec{0.f};
  DSPVector mStepVec{0.f};
  float mTargetValue{0};
  float mDyPerVector{1.f / 32};
  int mVectorsPerGlide{32};
  int mVectorsRemaining{-1};

 public:
  void setGlideTimeInSamples(float t)
  {
    mVectorsPerGlide = static_cast<int>(t / kFloatsPerDSPVector);
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
    if (mVectorsRemaining < 0)
    {
      // do nothing
    }
    else if (mVectorsRemaining == 0)
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
      mCurrVec = DSPVector(currentValue) + kUnityRampVec * mStepVec;

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

  void clear()
  {
    mCurrVec = 0.f;
    mStepVec = 0.f;
    mTargetValue = 0.f;
    mVectorsRemaining = -1;
  }
};

class SampleAccurateLinearGlide
{
  float mCurrValue{0.f};
  float mStepValue{0.f};
  float mTargetValue{0.f};
  int mSamplesPerGlide{32};
  float mDyPerSample{1.f / 32};
  int mSamplesRemaining{-1};

 public:
  void setGlideTimeInSamples(float t)
  {
    mSamplesPerGlide = static_cast<int>(t);
    if (mSamplesPerGlide < 1) mSamplesPerGlide = 1;
    mDyPerSample = 1.0f / mSamplesPerGlide;
  }

  // set the current value to the given value immediately, without gliding
  void setValue(float f)
  {
    mTargetValue = f;
    mSamplesRemaining = 0;
  }

  float nextSample(float f)
  {
    // set target value if different from current value.
    // const float currentValue = mCurrVec[kFloatsPerDSPVector - 1];
    if (f != mTargetValue)
    {
      mTargetValue = f;

      // start counter
      mSamplesRemaining = mSamplesPerGlide;
    }

    // process glide
    if (mSamplesRemaining < 0)
    {
      // do nothing
    }
    else if (mSamplesRemaining == 0)
    {
      // end glide: write target value to output vector
      mCurrValue = (mTargetValue);
      mStepValue = (0.f);
      mSamplesRemaining--;
    }
    else if (mSamplesRemaining == mSamplesPerGlide)
    {
      // start glide: get change in output value per sample
      mStepValue = (mTargetValue - mCurrValue) * mDyPerSample;
      mSamplesRemaining--;
    }
    else
    {
      // continue glide
      // Note that repeated adding will create some error in target value.
      // Because we return the target value explicity when we are done, this
      // won't be a problem in reasonably short glides.
      mCurrValue += mStepValue;
      mSamplesRemaining--;
    }

    return mCurrValue;
  }
  void clear()
  {
    mCurrValue = 0.f;
    mStepValue = 0.f;
    mTargetValue = 0.f;
    mSamplesRemaining = -1;
  }
};

}  // namespace ml
