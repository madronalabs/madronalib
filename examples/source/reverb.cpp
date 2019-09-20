// example of RtAudio wrapping low-level madronalib DSP code.
// The reverb in this example is the Aaltoverb algorithm (madronalabs.com/products/Aaltoverb) without the tone control and some filtering.

#include "RtAudioExample.h"

using namespace ml;

constexpr int kInputChannels = 2;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 44100;

// log projection for decay parameter
constexpr float kDecayLo = 0.8, kDecayHi = 20;
Projection unityToDecay(projections::intervalMap({0, 1}, {kDecayLo, kDecayHi}, projections::log({kDecayLo, kDecayHi})));

// parameter smoothers
LinearGlide mSmoothFeedback;
LinearGlide mSmoothDelay;

// reverb machinery
Allpass<PitchbendableDelay> mAp1, mAp2, mAp3, mAp4;
Allpass<PitchbendableDelay> mAp5, mAp6, mAp7, mAp8, mAp9, mAp10;
PitchbendableDelay mDelayL, mDelayR;

// feedback storage
DSPVector mvFeedbackL, mvFeedbackR;

void initializeReverb()
{
  // set fixed parameters for reverb
  mSmoothFeedback.setGlideTimeInSamples(0.1f*kSampleRate);
  mSmoothDelay.setGlideTimeInSamples(0.1f*kSampleRate);

  // set allpass filter coefficients
  mAp1.mGain = 0.75f;
  mAp2.mGain = 0.70f;
  mAp3.mGain = 0.625f;
  mAp4.mGain = 0.625f;
  mAp5.mGain = mAp6.mGain = 0.7f;
  mAp7.mGain = mAp8.mGain = 0.6f;
  mAp9.mGain = mAp10.mGain = 0.5f;

  // allocate delay memory
  mAp1.setMaxDelayInSamples(500.f);
  mAp2.setMaxDelayInSamples(500.f);
  mAp3.setMaxDelayInSamples(1000.f);
  mAp4.setMaxDelayInSamples(1000.f);
  mAp5.setMaxDelayInSamples(2600.f);
  mAp6.setMaxDelayInSamples(2600.f);
  mAp7.setMaxDelayInSamples(8000.f);
  mAp8.setMaxDelayInSamples(8000.f);
  mAp9.setMaxDelayInSamples(10000.f);
  mAp10.setMaxDelayInSamples(10000.f);
  mDelayL.setMaxDelayInSamples(3500.f);
  mDelayR.setMaxDelayInSamples(3500.f);
}

// do filter processing in DSPVector-sized chunks.
DSPVectorArray<2> processVectors(const DSPVectorArray<2>& inputVectors)
{
  const float sr = kSampleRate;
  const float RT60const = 0.001f;

  // size and decay parameters from 0-1. It will be more interesting to change these over time in some way.
  float sizeU = 0.5f;
  float decayU = 0.5f;

  // generate delay and feedback scalars
  float decayTime = unityToDecay(decayU);
  float decayIterations = decayTime/(sizeU*0.5);
  float feedback = (decayU < 1.0f) ? powf(RT60const, 1.0f/decayIterations) : 1.0f;

  // generate smoothed delay time and feedback gain vectors
  DSPVector vSmoothDelay = mSmoothDelay(sizeU*2.0f);
  DSPVector vSmoothFeedback = mSmoothFeedback(feedback);

  // get smoothed allpass times
  DSPVector vMin(kFloatsPerDSPVector);
  DSPVector vt1 = max(DSPVector(0.00476*sr*vSmoothDelay), vMin);
  DSPVector vt2 = max(DSPVector(0.00358*sr*vSmoothDelay), vMin);
  DSPVector vt3 = max(DSPVector(0.00973*sr*vSmoothDelay), vMin);
  DSPVector vt4 = max(DSPVector(0.00830*sr*vSmoothDelay), vMin);
  DSPVector vt5 = max(DSPVector(0.029*sr*vSmoothDelay), vMin);
  DSPVector vt6 = max(DSPVector(0.021*sr*vSmoothDelay), vMin);
  DSPVector vt7 = max(DSPVector(0.078*sr*vSmoothDelay), vMin);
  DSPVector vt8 = max(DSPVector(0.090*sr*vSmoothDelay), vMin);
  DSPVector vt9 = max(DSPVector(0.111*sr*vSmoothDelay), vMin);
  DSPVector vt10 = max(DSPVector(0.096*sr*vSmoothDelay), vMin);

  // sum stereo inputs and diffuse with allpass filters
  DSPVector monoInput = (inputVectors.constRow(0) + inputVectors.constRow(1));
  DSPVector diffusedInput = mAp4(mAp3(mAp2(mAp1(monoInput, vt1), vt2), vt3), vt4);

  // get delay times in samples, subtracting the constant delay of one DSPVector and clamping
  DSPVector vDelayTimeL = max(vSmoothDelay*DSPVector(0.0313*sr) - vMin, DSPVector(0.f));
  DSPVector vDelayTimeR = max(vSmoothDelay*DSPVector(0.0371*sr) - vMin, DSPVector(0.f));

  // sum diffused input with feedback, and apply late diffusion
  DSPVector vTapL = mAp7(mAp5(diffusedInput + mDelayL(mvFeedbackL, vDelayTimeL), vt5), vt7);
  DSPVector vTapR = mAp8(mAp6(diffusedInput + mDelayR(mvFeedbackR, vDelayTimeR), vt6), vt8);

  mvFeedbackR = mAp9(vTapL, vt9)*vSmoothFeedback;
  mvFeedbackL = mAp10(vTapR, vt10)*vSmoothFeedback;

  return append(vTapL, vTapR);
}

int main( int argc, char *argv[] )
{
  using processFnType = std::function<DSPVectorArray<kOutputChannels>(const DSPVectorArray<kInputChannels>&)>;

  initializeReverb();

  processFnType processFn([&](const DSPVectorArray<kInputChannels> inputVectors) { return processVectors(inputVectors); });

  return RunRtAudioExample(kInputChannels, kOutputChannels, kSampleRate, &callProcessVectorsBuffered<kInputChannels, kOutputChannels>, &processFn);
}
