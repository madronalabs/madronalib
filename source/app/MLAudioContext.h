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

class AudioContext final {
public:
  // AudioContext::ProcessTime maintains the current time in a DSP process and can track
  // the time in the host application if there is one.
  class ProcessTime
  {
  public:
    ProcessTime() = default;
    ~ProcessTime() = default;

    // Set the time and bpm. The time refers to the start of the current engine processing block.
    void setTimeAndRate(const double ppqPos, const double bpmIn, bool isPlaying, double sampleRateIn);
           
    // clear state
    void clear();

    // generate phasors from the input parameters
    void processVector(int startOffset);

    // phase signal to read TODO rename
    DSPVector _quarterNotesPhase;

    double bpm{0};
    double sampleRate{0};
    uint64_t samplesSinceStart{0};

  private:
    float _omega{0};
    bool _playing1{false};
    bool _active1{false};
    double _dpdt{0};
    size_t _samplesSincePreviousTime{0};
    double _ppqPos1{-1.};
    double _ppqPhase1{0};
  };

  AudioContext(size_t nInputs, size_t nOutputs, int sr);
  ~AudioContext() = default;
  
  void clear();

  // update everything needed to create a new vector of context signals.
  // startOffset is the start frame of the vector in the host buffer.
  void processVector(int startOffset);
  
  void setInputPolyphony(int voices) { eventsToSignals.setPolyphony(voices); }
  int getInputPolyphony() { return eventsToSignals.getPolyphony(); }

  void updateTime(const double ppqPos, const double bpmIn, bool isPlaying, double sampleRateIn);
  DSPVector getBeatPhase() { return currentTime._quarterNotesPhase; }

  void addInputEvent(const Event& e);
  void clearInputEvents() { eventsToSignals.clearEvents(); }

  void setInputPitchBend(float p) { eventsToSignals.setPitchBendInSemitones(p); }
  void setInputMPEPitchBend(float p) { eventsToSignals.setMPEPitchBendInSemitones(p); }
  void setInputGlideTimeInSeconds(float s) { eventsToSignals.setGlideTimeInSeconds(s); }
  void setInputDriftAmount(float d) { eventsToSignals.setDriftAmount(d); }
  void setInputUnison(bool u) { eventsToSignals.setUnison(u); }
  void setInputProtocol(Symbol p) { eventsToSignals.setProtocol(p); }
  void setInputModCC(int p) { eventsToSignals.setModCC(p); }
  
  // by giving clients only a const Voice&, we are letting them inspect anything about Voices, but
  // not modify them. This seems like a useful pattern for any object owning output-containing structs.
  const EventsToSignals::Voice& getInputVoice(int n) { return eventsToSignals.getVoice(n); }
  
  int getNewestInputVoice() { return eventsToSignals.getNewestVoice(); }
  DSPVector getInputController(size_t n) const;
  
  // clients can access these directly to do processing
  DSPVectorDynamic inputs;
  DSPVectorDynamic outputs;
  int sampleRate{0};
  
private:
  ProcessTime currentTime;
  ml::EventsToSignals eventsToSignals;
};


} // ml
