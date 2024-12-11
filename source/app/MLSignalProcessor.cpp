// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalProcessor.h"

using namespace ml;

// SignalProcessor::PublishedSignal

SignalProcessor::PublishedSignal::PublishedSignal(int maxFrames, int maxVoices, int channels, int octavesDown)
  : _channels(channels), maxFrames_(maxFrames), octavesDown_(octavesDown)
{
  voiceRotateBuffer.resize(maxFrames * channels);
  _buffer.resize(maxFrames * channels * maxVoices);
}

size_t SignalProcessor::PublishedSignal::readLatest(float* pDest, size_t framesRequested)
{
  size_t floatsAvailable = getReadAvailable();
  if (floatsAvailable > framesRequested*_channels)
  {
    _buffer.discard(floatsAvailable - framesRequested*_channels);
  }
  
  return _buffer.read(pDest, framesRequested*_channels);
}

void SignalProcessor::PublishedSignal::peekLatest(float* pDest, size_t framesRequested)
{
  _buffer.peekMostRecent(pDest, framesRequested * _channels);
}

size_t SignalProcessor::PublishedSignal::read(float* pDest, size_t framesRequested)
{
  return _buffer.read(pDest, framesRequested * _channels);
}

// SignalProcessor::ProcessTime

// Set the time and bpm. The time refers to the start of the current engine processing block.
void SignalProcessor::ProcessTime::setTimeAndRate(const double secs, const double ppqPos,
                                                  const double bpm, bool isPlaying,
                                                  double sampleRate)
{
  // working around a bug I can't reproduce, so I'm covering all the bases.
  if (((ml::isNaN(ppqPos)) || (ml::isInfinite(ppqPos))) ||
      ((ml::isNaN(bpm)) || (ml::isInfinite(bpm))) || ((ml::isNaN(secs)) || (ml::isInfinite(secs))))
  {
    // debug << "PluginProcessor::ProcessTime::setTimeAndRate: bad input! \n";
    return;
  }

  bool active = (_ppqPos1 != ppqPos) && isPlaying;
  bool justStarted = isPlaying && !_playing1;

  double ppqPhase = 0.;

  if (active)
  {
    if (ppqPos > 0.f)
    {
      ppqPhase = ppqPos - floor(ppqPos);
    }
    else
    {
      ppqPhase = ppqPos;
    }

    _omega = ppqPhase;

    if (justStarted)
    {
      // just start at 0 and don't attempt to match the playhead position.
      // this works well when we start at any 1/4 note.
      // there is still some weirdness when we try to lock onto other 16ths.
      _omega = 0.;
      // std::cout << "phasor START: " << mOmega << "\n";
      _dpdt = 0.;
      _dsdt = 0.;
    }
    else
    {
      double dPhase = ppqPhase - _ppqPhase1;
      if (dPhase < 0.)
      {
        dPhase += 1.;
      }
      _dpdt = ml::clamp(dPhase / static_cast<double>(_samplesSincePreviousTime), 0., 1.);
      _dsdt = static_cast<float>(1. / sampleRate);
    }

    _secondsCounter = secs;
    _secondsPhaseCounter = fmodl(secs, 1.0);
  }
  else
  {
    _omega = -1.;
    _dpdt = 0.;
    _secondsCounter = -1.;
    _dsdt = 0.;
    _secondsPhaseCounter = -1.;
  }

  _ppqPos1 = ppqPos;
  _ppqPhase1 = ppqPhase;
  _active1 = active;
  _playing1 = isPlaying;
  _samplesSincePreviousTime = 0;
}

void SignalProcessor::ProcessTime::clear(void)
{
  _dpdt = 0.;
  _dsdt = 0.;
  _active1 = false;
  _playing1 = false;
}

// generate phasors from the input parameters
void SignalProcessor::ProcessTime::process()
{
  auto _fomega = (double)_omega;
  for (int n = 0; n < kFloatsPerDSPVector; ++n)
  {
    _quarterNotesPhase[n] = (float)_fomega;
    _fomega += _dpdt;
    if (_fomega > 1.f)
    {
      _fomega -= 1.f;
    }
  }
  for (int n = 0; n < kFloatsPerDSPVector; ++n)
  {
    _seconds[n] = (float)_secondsCounter;
    _secondsCounter += _dsdt;
    _secondsPhase[n] = (float)_secondsPhaseCounter;
    _secondsPhaseCounter += _dsdt;
  }
  _samplesSincePreviousTime += kFloatsPerDSPVector;
}
