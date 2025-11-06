// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// AudioTask: adaptor from RtAudio's main loop to madronalib vector processing

#pragma once

#include "MLSignalProcessBuffer.h"
#include "MLSignalProcessor.h"
#include "MLAudioContext.h"
#include "mldsp.h"

namespace ml
{

// AudioTask: run an audio processing function in a context, with a state.
// This is where any external audio I/O from a host or run loop is buffered into
// kFloatsPerSignalVector-sized chunks.

class AudioTask
{
  // the maximum amount of input frames that can be proceesed at once. This determines the
  // maximum signal vector size of the plugin host or enclosing app.
  static constexpr int kMaxBlockSize{4096};

 public:
  AudioTask(AudioContext* ctx, SignalProcessFn procFn, void* procState);

  ~AudioTask();

  int startAudio();
  void stopAudio();
  int runConsoleApp();

 private:
  struct Impl;
  std::unique_ptr<Impl> pImpl;
};

}  // namespace ml
