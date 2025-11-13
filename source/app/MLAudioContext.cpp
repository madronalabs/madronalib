//
// Created by Randy Jones on 2/27/25.
//

#include "MLAudioContext.h"

namespace ml
{

// AudioContext::ProcessTime

// Set the time and bpm. The time refers to the start of the current processing block.
// In a plugin, this should be called before each processing block with the latest info from the
// host. In an app, this can be called only when there are time / rate changes.

void AudioContext::ProcessTime::setTimeAndRate(const double ppqPos, const double bpmIn,
                                               bool isPlaying, double sampleRateIn)
{
  // working around a bug I can't reproduce, so I'm covering all the bases.
  if (((ml::isNaN(ppqPos)) || (ml::isInfinite(ppqPos))) ||
      ((ml::isNaN(bpmIn)) || (ml::isInfinite(bpmIn))))
  {
    // debug << "PluginProcessor::ProcessTime::setTimeAndRate: bad input! \n";
    return;
  }

  sampleRate = sampleRateIn;
  bpm = bpmIn;
  bool active = (ppqPos1_ != ppqPos) && isPlaying;
  bool justStarted = isPlaying && !playing1_;

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

    omega_ = ppqPhase;

    if (justStarted)
    {
      // just start at 0 and don't attempt to match the playhead position.
      // this works well when we start at any 1/4 note.
      // there is still some weirdness when we try to lock onto other 16ths.
      samplesSinceStart = 0;
      omega_ = 0.;
      double dsdt = (1. / sampleRate);
      double minutesPerSample = dsdt / 60.;
      dpdt_ = bpm * minutesPerSample;
    }
    else
    {
      double dPhase = ppqPhase - ppqPhase1_;
      if (dPhase < 0.)
      {
        dPhase += 1.;
      }
      dpdt_ = ml::clamp(dPhase / static_cast<double>(samplesSincePreviousTime_), 0., 1.);
    }
  }
  else
  {
    omega_ = -1.;
    dpdt_ = 0.;
  }

  ppqPos1_ = ppqPos;
  ppqPhase1_ = ppqPhase;
  active1_ = active;
  playing1_ = isPlaying;
  samplesSincePreviousTime_ = 0;
}

void AudioContext::ProcessTime::clear(void)
{
  dpdt_ = 0.;
  active1_ = false;
  playing1_ = false;
}

// generate phasors from the input parameters
void AudioContext::ProcessTime::processVector(int startOffset)
{
  for (int n = 0; n < kFloatsPerDSPVector; ++n)
  {
    quarterNotesPhase_[n] = omega_;
    omega_ += dpdt_;
    if (omega_ > 1.f)
    {
      omega_ -= 1.f;
    }
  }
  samplesSincePreviousTime_ += kFloatsPerDSPVector;
  samplesSinceStart += kFloatsPerDSPVector;
}

AudioContext::AudioContext(size_t nInputs, size_t nOutputs) : inputs(nInputs), outputs(nOutputs) {}

AudioContext::AudioContext(size_t nInputs, size_t nOutputs, int rate)
    : inputs(nInputs), outputs(nOutputs)
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

void AudioContext::addInputEvent(const Event& e) { eventsToSignals.addEvent(e); }

void AudioContext::updateTime(const double ppqPos, const double bpmIn, bool isPlaying,
                              double sampleRateIn)
{
  currentTime.setTimeAndRate(ppqPos, bpmIn, isPlaying, sampleRateIn);
}

}  // namespace ml
