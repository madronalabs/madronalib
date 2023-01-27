// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "madronalib.h"
#include "mldsp.h"

namespace ml
{

// SynthInput processes different types of events and generates bundles of signals to
// control synthesizers.
//
class SynthInput final
{
  enum
  {
    kPitch = 0,
    kVelocity,
    kVoice,
    kAftertouch,
    kMod,
    kX,
    kY,
    kElapsedTime,
    kNumVoiceChannels
  };

  
 public:
  SynthInput() = default;
  ~SynthInput() = default;
  
  // create voices
  size_t resize(size_t voices);
  
  // clear all voices and reset state.
  void reset();
  
  // process VST3 event input
  processEvents(IEventList* events);
  
  

};

}  // namespace ml
