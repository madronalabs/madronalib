// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// example of RtAudio wrapping low-level madronalib DSP code.

#include <chrono>
#include <iostream>
#include <thread>

#include "madronalib.h"

using namespace ml;

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kOutputGain = 0.1f;

struct SineExampleState
{
  SineGen s1, s2;
};

// sineProcess() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
void sineProcess(AudioContext* ctx, void *state)
{
  // at the beginning of the main process function we need to cast the void* to
  // the type of our state. Making AudioTask a template would have been an alternative
  // to this but would have added a lot of template code behind the scenes.
  auto procState = static_cast<SineExampleState*>(state);

  // Running the sine generators makes DSPVectors as output.
  // The input parameter is omega: the frequency in Hz divided by the sample rate.
  // The output sines are multiplied by the gain.
  ctx->outputs[0] = procState->s1(220.f/kSampleRate)*kOutputGain;
  ctx->outputs[1] = procState->s2(275.f/kSampleRate)*kOutputGain;
}

int main()
{
  SineExampleState state;
  AudioContext ctx(kInputChannels, kOutputChannels, kSampleRate);
  AudioTask sineExample(&ctx, sineProcess, &state);

  sineExample.startAudio();

  sineExample.stopAudio();

  return sineExample.runConsoleApp();
}

