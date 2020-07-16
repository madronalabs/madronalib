// example of RtAudio wrapping low-level madronalib DSP code.

#include "MLDSPFunctional.h"

#include "RtAudioExample.h"

using namespace ml;

#ifdef __WIN32__
fix this
#endif

// Mac OS note: need to ask for microphone access if this is nonzero!
constexpr int kInputChannels = 0; // NOTE: 0 will not compile on Windows-- TODO

constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 44100;
constexpr float kOutputGain = 0.1f;

// sine generators.
SineGen s1, s2;

// processVectors() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
DSPVectorArray<kOutputChannels> processVectors(const DSPVectorArray<kInputChannels>& inputVectors)
{
  // Running the sine generators makes DSPVectors as output.
  // The input parameter is omega: the frequency in Hz divided by the sample rate.
  // The output sines are multiplied by the gain.
  auto sineL = s1(220.f/kSampleRate)*kOutputGain;
  auto sineR = s2(275.f/kSampleRate)*kOutputGain;

  // appending the two DSPVectors makes a DSPVectorArray<2>: our stereo output.
  return append(sineL, sineR);
}

int main( int argc, char *argv[] )
{
  // This code adapts the RtAudio loop to our buffered processing and runs the example.
  auto RtAudioCallbackFnPtr = &callProcessVectorsBuffered< kInputChannels, kOutputChannels >;
  return RunRtAudioExample< kInputChannels, kOutputChannels >(kSampleRate, RtAudioCallbackFnPtr, &processVectors);
}
