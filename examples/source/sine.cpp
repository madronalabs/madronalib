// example of RtAudio wrapping low-level madronalib DSP code.

#include "RtAudioExample.h"

using namespace ml;

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 44100;

SineGen s1, s2;

// do filter processing in DSPVector-sized chunks.
DSPVectorArray<kOutputChannels> processVectors(const DSPVectorArray<kInputChannels>& inputVectors)
{
  float gain = 0.1f;
  return append(s1(220.f/kSampleRate)*gain, s2(275.f/kSampleRate)*gain);
}

int main( int argc, char *argv[] )
{
  using processFnType = std::function<DSPVectorArray<kOutputChannels>(const DSPVectorArray<kInputChannels>&)>;

  processFnType processFn([&](const DSPVectorArray<kInputChannels> inputVectors) { return processVectors(inputVectors); });

  return RunRtAudioExample(kInputChannels, kOutputChannels, kSampleRate, &callProcessVectorsBuffered<kInputChannels, kOutputChannels>, &processFn);
}
