//
// Created by Randy Jones on 2/27/25.
//

#pragma once

#include "MLDSPOps.h"
#include "MLEventsToSignals.h"

#include <cstdlib>
#include <functional>

using namespace ml;
namespace ml
{

// AudioContext: where our signal processors meet the rest of the world.
// an AudioContext defines the sample rate and provides audio and event I/O.

using MainInputs = const DSPVectorDynamic&;
using MainOutputs = DSPVectorDynamic&;

class AudioContext final
{
 public:
  // AudioContext::ProcessTime maintains the current time in a DSP process and can track
  // the time in the host application if there is one.
  class ProcessTime
  {
   public:
    ProcessTime() = default;
    ~ProcessTime() = default;

    // Set the time and bpm. The time refers to the start of the current engine processing block.
    void setTimeAndRate(const double ppqPos, const double bpmIn, bool isPlaying,
                        double sampleRateIn);

    // clear state
    void clear();

    // generate phasors from the input parameters
    void processVector(int startOffset);

    // externally readable values updated by processVector()
    DSPVector quarterNotesPhase_;
    double bpm{0};
    double sampleRate{0};
    uint64_t samplesSinceStart{0};

   private:
    float omega_{0};
    bool playing1_{false};
    bool active1_{false};
    double dpdt_{0};
    size_t samplesSincePreviousTime_{0};
    double ppqPos1_{-1.};
    double ppqPhase1_{0};
  };

  AudioContext(size_t nInputs, size_t nOutputs);
  AudioContext(size_t nInputs, size_t nOutputs, int rate);
  ~AudioContext() = default;

  void clear();

  // update everything needed to create a new vector of context signals.
  // startOffset is the start frame of the vector in the host buffer.
  void processVector(int startOffset);

  void setSampleRate(int r);

  void setInputPolyphony(int voices) { eventsToSignals.setPolyphony(voices); }
  size_t getInputPolyphony() { return eventsToSignals.getPolyphony(); }

  void updateTime(const double ppqPos, const double bpmIn, bool isPlaying, double sampleRateIn);
  DSPVector getBeatPhase() { return currentTime.quarterNotesPhase_; }

  void addInputEvent(const Event& e);
  void clearInputEvents() { eventsToSignals.clearEvents(); }

  void setInputPitchBend(float p) { eventsToSignals.setPitchBendInSemitones(p); }
  void setInputMPEPitchBend(float p) { eventsToSignals.setMPEPitchBendInSemitones(p); }
  void setInputGlideTimeInSeconds(float s) { eventsToSignals.setPitchGlideInSeconds(s); }
  void setInputDriftAmount(float d) { eventsToSignals.setDriftAmount(d); }
  void setInputUnison(bool u) { eventsToSignals.setUnison(u); }
  void setInputProtocol(Symbol p) { eventsToSignals.setProtocol(p); }
  void setInputModCC(int p) { eventsToSignals.setModCC(p); }
  const EventsToSignals::Voice& getInputVoice(int n) { return eventsToSignals.getVoice(n); }

  int getNewestInputVoice() { return eventsToSignals.getNewestVoice(); }
  DSPVector getInputController(size_t n) const;

  double getSampleRate() { return currentTime.sampleRate; }
  const ProcessTime& getTimeInfo() { return currentTime; }

  // clients can access these directly to do processing
  DSPVectorDynamic inputs;
  DSPVectorDynamic outputs;

 private:
  ProcessTime currentTime;
  ml::EventsToSignals eventsToSignals;
};

}  // namespace ml
