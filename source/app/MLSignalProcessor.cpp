// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalProcessor.h"

using namespace ml;

// SignalProcessor::PublishedSignal

SignalProcessor::PublishedSignal::PublishedSignal(int maxFrames, int maxVoices, int channels,
                                                  int octavesDown)
    : channels_(channels), maxFrames_(maxFrames), octavesDown_(octavesDown)
{
  voiceRotateBuffer.resize(maxFrames * channels);
  buffer_.resize(maxFrames * channels * maxVoices);
}

size_t SignalProcessor::PublishedSignal::readLatest(float* pDest, size_t framesRequested)
{
  size_t floatsAvailable = getReadAvailable();
  if (floatsAvailable > framesRequested * channels_)
  {
    buffer_.discard(floatsAvailable - framesRequested * channels_);
  }

  return buffer_.read(pDest, framesRequested * channels_);
}

void SignalProcessor::PublishedSignal::peekLatest(float* pDest, size_t framesRequested)
{
  buffer_.peekMostRecent(pDest, framesRequested * channels_);
}

size_t SignalProcessor::PublishedSignal::read(float* pDest, size_t framesRequested)
{
  return buffer_.read(pDest, framesRequested * channels_);
}
