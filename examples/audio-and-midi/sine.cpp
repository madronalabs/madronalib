// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// example of RtAudio wrapping low-level madronalib DSP code.

#include "MLAudioProcessor.h"

using namespace ml;

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kOutputGain = 0.1f;

// sine generators.
SineGen s1, s2;

// sineProcess() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
void sineProcess(MainInputs unused, MainOutputs outputs, void *stateDataUnused)
{
  // Running the sine generators makes DSPVectors as output.
  // The input parameter is omega: the frequency in Hz divided by the sample rate.
  // The output sines are multiplied by the gain.
  outputs[0] = s1(220.f/kSampleRate)*kOutputGain;
  outputs[1] = s2(275.f/kSampleRate)*kOutputGain;
}

int main()
{
  // The AudioProcessor object adapts the RtAudio loop to our buffered processing and runs the example.
  AudioProcessor sineExample(kInputChannels, kOutputChannels, kSampleRate, sineProcess);
  return sineExample.run();
}
