//
// Created by Randy Jones on 2/27/25.
//

#include "MLAudioContext.h"

namespace ml
{

// AudioContext::ProcessTime

// Set the time and bpm. The time refers to the start of the current processing block.
// In a plugin, this should be called before each processing block with the latest info from the host.
// In an app, this can be called only when there are time / rate changes.

void AudioContext::ProcessTime::setTimeAndRate(const double ppqPos, const double bpmIn,
  bool isPlaying, double sampleRateIn)
{
  // working around a bug I can't reproduce, so I'm covering all the bases.
  if (((ml::isNaN(ppqPos)) || (ml::isInfinite(ppqPos))) ||
      ((ml::isNaN(bpmIn)) || (ml::isInfinite(bpmIn))) )
  {
    // debug << "PluginProcessor::ProcessTime::setTimeAndRate: bad input! \n";
    return;
  }

  sampleRate = sampleRateIn;
  bpm = bpmIn;
  bool active = (_ppqPos1 != ppqPos) && isPlaying;
  bool justStarted = isPlaying && !_playing1;

  double ppqPhase = 0.;
  if(active)
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
      samplesSinceStart = 0;
      _omega = 0.;
      double dsdt = (1. / sampleRate);
      double minutesPerSample = dsdt/60.;
      _dpdt = bpm*minutesPerSample;
    }
    else
    {
      double dPhase = ppqPhase - _ppqPhase1;
      if (dPhase < 0.)
      {
        dPhase += 1.;
      }
      _dpdt = ml::clamp(dPhase / static_cast<double>(_samplesSincePreviousTime), 0., 1.);
    }
  }
  else
  {
    _omega = -1.;
    _dpdt = 0.;
  }

  _ppqPos1 = ppqPos;
  _ppqPhase1 = ppqPhase;
  _active1 = active;
  _playing1 = isPlaying;
  _samplesSincePreviousTime = 0;
}

void AudioContext::ProcessTime::clear(void)
{
  _dpdt = 0.;
  _active1 = false;
  _playing1 = false;
}

// generate phasors from the input parameters
void AudioContext::ProcessTime::processVector(int startOffset)
{
  for (int n = 0; n < kFloatsPerDSPVector; ++n)
  {
    _quarterNotesPhase[n] = _omega;
    _omega += _dpdt;
    if (_omega > 1.f)
    {
      _omega -= 1.f;
    }
  }
  _samplesSincePreviousTime += kFloatsPerDSPVector;
  samplesSinceStart += kFloatsPerDSPVector;
}

AudioContext::AudioContext(size_t nInputs, size_t nOutputs) :
inputs(nInputs), outputs(nOutputs)
{}

AudioContext::AudioContext(size_t nInputs, size_t nOutputs, int rate) :
inputs(nInputs), outputs(nOutputs)
{
  setSampleRate(rate);
}

void AudioContext::setSampleRate(int r)
{
  currentTime.sampleRate = r;
  eventsToSignals.setSampleRate(r);
}

void AudioContext::clear()
{
  currentTime.clear();
  eventsToSignals.clear();
}

void AudioContext::processVector(int startOffset)
{
  currentTime.processVector(startOffset);
  eventsToSignals.processVector(startOffset);
}

DSPVector AudioContext::getInputController(size_t n) const
{
  return eventsToSignals.getController(n).output;
}

void AudioContext::addInputEvent(const Event& e)
{
  eventsToSignals.addEvent(e);
}

void AudioContext::updateTime(const double ppqPos, const double bpmIn, bool isPlaying, double sampleRateIn)
{
  currentTime.setTimeAndRate(ppqPos, bpmIn, isPlaying, sampleRateIn);
}

} // ml
