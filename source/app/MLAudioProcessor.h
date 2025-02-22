// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// AudioProcessor: adaptor from RtAudio's main loop to madronalib vector processing

#pragma once
#include "MLSignalProcessor.h"
#include "mldsp.h"

namespace ml
{

class AudioProcessor : public SignalProcessor, public Actor
{
public:
  AudioProcessor(size_t nInputs, size_t nOutputs, int sampleRate,
                   ProcessVectorFn processFn = nullptr, void* state = nullptr);

  ~AudioProcessor();

  int startAudio();
  int run();
  void waitForEnterKey();
  void stopAudio();

  // Actor implementation
  void onMessage(Message msg) override;

private:
  struct Impl;
  std::unique_ptr< Impl > pImpl;
};

} // namespace ml