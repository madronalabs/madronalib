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
    void process();

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

  // update everything needed to create a new vector of context signals.
  void processVector();

  ProcessTime currentTime;
  ml::EventsToSignals eventsToSignals;

  DSPVectorDynamic inputs;
  DSPVectorDynamic outputs;
  int sampleRate{0};
};


} // ml
