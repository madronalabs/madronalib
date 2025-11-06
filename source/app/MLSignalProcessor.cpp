// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalProcessor.h"

using namespace ml;

// SignalProcessor::PublishedSignal

SignalProcessor::PublishedSignal::PublishedSignal(int maxFrames, int maxVoices, int channels,
                                                  int octavesDown)
    : _channels(channels), maxFrames_(maxFrames), octavesDown_(octavesDown)
{
  voiceRotateBuffer.resize(maxFrames * channels);
  _buffer.resize(maxFrames * channels * maxVoices);
}

size_t SignalProcessor::PublishedSignal::readLatest(float* pDest, size_t framesRequested)
{
  size_t floatsAvailable = getReadAvailable();
  if (floatsAvailable > framesRequested * _channels)
  {
    _buffer.discard(floatsAvailable - framesRequested * _channels);
  }

  return _buffer.read(pDest, framesRequested * _channels);
}

void SignalProcessor::PublishedSignal::peekLatest(float* pDest, size_t framesRequested)
{
  _buffer.peekMostRecent(pDest, framesRequested * _channels);
}

size_t SignalProcessor::PublishedSignal::read(float* pDest, size_t framesRequested)
{
  return _buffer.read(pDest, framesRequested * _channels);
}
