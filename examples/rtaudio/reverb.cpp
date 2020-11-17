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

// processVectors() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
DSPVectorArray<kOutputChannels> processVectors(const DSPVectorArray<kInputChannels>& inputVectors)
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

  // get the minimum possible delay in samples, which is the length of a DSPVector.
  DSPVector vMin(kFloatsPerDSPVector);

  // get smoothed allpass times in samples
  DSPVector delayParamInSamples = sr*vSmoothDelay;
  DSPVector vt1 = max(0.00476*delayParamInSamples, vMin);
  DSPVector vt2 = max(0.00358*delayParamInSamples, vMin);
  DSPVector vt3 = max(0.00973*delayParamInSamples, vMin);
  DSPVector vt4 = max(0.00830*delayParamInSamples, vMin);
  DSPVector vt5 = max(0.029*delayParamInSamples, vMin);
  DSPVector vt6 = max(0.021*delayParamInSamples, vMin);
  DSPVector vt7 = max(0.078*delayParamInSamples, vMin);
  DSPVector vt8 = max(0.090*delayParamInSamples, vMin);
  DSPVector vt9 = max(0.111*delayParamInSamples, vMin);
  DSPVector vt10 = max(0.096*delayParamInSamples, vMin);

  // sum stereo inputs and diffuse with four allpass filters in series
  DSPVector monoInput = (inputVectors.constRow(0) + inputVectors.constRow(1));
  DSPVector diffusedInput = mAp4(mAp3(mAp2(mAp1(monoInput, vt1), vt2), vt3), vt4);

  // get delay times in samples, subtracting the constant delay of one DSPVector and clamping to zero
  DSPVector vDelayTimeL = max(0.0313*delayParamInSamples - vMin, DSPVector(0.f));
  DSPVector vDelayTimeR = max(0.0371*delayParamInSamples - vMin, DSPVector(0.f));

  // sum diffused input with feedback, and apply late diffusion of two more allpass filters to each channel
  DSPVector vTapL = mAp7(mAp5(diffusedInput + mDelayL(mvFeedbackL, vDelayTimeL), vt5), vt7);
  DSPVector vTapR = mAp8(mAp6(diffusedInput + mDelayR(mvFeedbackR, vDelayTimeR), vt6), vt8);

  // apply final allpass filter and gain, and store the feedback
  mvFeedbackR = mAp9(vTapL, vt9)*vSmoothFeedback;
  mvFeedbackL = mAp10(vTapR, vt10)*vSmoothFeedback;

  // append the left and right taps and return the stereo output
  return concatRows(vTapL, vTapR);
}

int main( int argc, char *argv[] )
{
  initializeReverb();

  // This code adapts the RtAudio loop to our buffered processing and runs the example.
  RtAudioExample< kInputChannels, kOutputChannels > reverbExample(kSampleRate, &processVectors);
  return reverbExample.run();
}
