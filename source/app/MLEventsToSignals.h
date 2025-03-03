// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

//#include "madronalib.h"
#include "mldsp.h"
#include "MLEvent.h"
#include "MLQueue.h"

namespace ml
{

// rows per voice output signal.
enum VoiceOutputSignals
{
  kPitch = 0,
  kGate,
  kVoice,
  kX,
  kY,
  kZ,
  kMod,
  kElapsedTime,
  kNumVoiceOutputRows
};

struct KeyState
{
  enum PlayingState
  {
    kOff,
    kOn,
    kSustain
  };
  PlayingState state{kOff};
  float pitch{0.f};
  uint32_t noteOnIndex{0};
};

// EventsToSignals processes different types of events and generates bundles of signals to
// control synthesizers.

class EventsToSignals final
{
public:
  static constexpr size_t kMaxVoices{16};
  static constexpr size_t kMaxEventsPerVector{128};
  static constexpr size_t kMaxPhysicalKeys{128};
  static constexpr size_t kNumControllers{128};

  static constexpr float kGlideTimeSeconds{0.02f};
  static constexpr float kControllerGlideTimeSeconds{0.02f};
  static constexpr float kDriftTimeSeconds{8.0f};
  static constexpr float kDriftScale{0.02f};

  explicit EventsToSignals(int sr);
  ~EventsToSignals();

  size_t setPolyphony(size_t n);
  size_t getPolyphony();

  int getNewestVoice() const { return newestVoice_; }

  // clear all voices and queued events and reset state.
  void reset();

  // just reset time outputs
  void resetTimes();

  // add an event to the queue.
  void addEvent(const Event& e);

  // process all events in queue and generate output signals.
  void process();

  void setPitchBendInSemitones(float f);
  void setGlideTimeInSeconds(float f);
  void setDriftAmount(float f);
  void setUnison(bool b);

  #pragma mark -
  
  // Voice: a voice that can play.
  //
  struct Voice
  {
    Voice() = default;
    ~Voice() = default;

    void setParams(float pitchGlideInSeconds, float drift, float sr);

    void reset(int voiceIdx);
    
    void resetTime(int voiceIdx);
    
    // send to start processing a new buffer.
    void beginProcess(float sr);
    
    // send a note on, off update or sustain event to the voice.
    void writeNoteEvent(const Event& e, float sr, bool doGlide = true);

    // write all current info to the end of the current buffer.
    // add pitchBend to pitch.
    void endProcess(float pitchBend, float sr);
    
    size_t nextFrameToProcess{0};

    // instantaneous values, written during event processing
    float currentVelocity{0};
    float currentPitch{0};
    float currentPitchBend{0};
    float currentMod{0};
    float currentX{0};
    float currentY{0};
    float currentZ{0};

    size_t creatorKeyNumber{0}; // physical key or touch # of creator. 0 = undefined.
    uint32_t ageInSamples{0};
    uint32_t ageStep{0};

    SampleAccurateLinearGlide pitchGlide;
    LinearGlide pitchBendGlide;
    LinearGlide modGlide;
    LinearGlide xGlide;
    LinearGlide yGlide;
    LinearGlide zGlide;
    int pitchGlideTimeInSamples{0};
    bool inhibitPitchGlide{0};

    // drift generates a wandering signal on [0, 1] then is scaled and added to pitch
    // TODO encapsulate this as DrunkenWalkGen
    RandomScalarSource driftSource;
    LinearGlide pitchDriftGlide;
    int driftCounter{0};
    float currentDriftValue{0};
    float driftAmount{0};
    int nextDriftTimeInSamples{0};
    
    // output signals (velocity, pitch, voice... )
    DSPVectorArray< kNumVoiceOutputRows > outputs;
  };

  struct SmoothedController
  {
    LinearGlide glide;
    DSPVector output;
    float rawValue;

    void process();
  };

  // voices, containing signals for clients to read directly.
  std::vector< Voice > voices;

  // output values for continuous controllers.
  std::vector< SmoothedController > controllers;
  
private:

  size_t countHeldNotes();
  void processEvent(const Event &eventParam);
  void processNoteOnEvent(const Event& event);
  void processNoteOffEvent(const Event& event);
  void processNoteUpdateEvent(const Event& event);
  void processControllerEvent(const Event& event);
  void processPitchWheelEvent(const Event& event);
  void processNotePressureEvent(const Event& event);
  void processChannelPressureEvent(const Event& event);
  void processSustainEvent(const Event& event);
  int findFreeVoice();
  int findVoiceToSteal(Event e);
  int findNearestVoice(int note);
  
  std::array< KeyState, kMaxPhysicalKeys > keyStates_;
  Queue< Event > eventQueue_;
  int polyphony_{0};
  int lastFreeVoiceFound_{-1};
  int newestVoice_{-1};
  bool sustainPedalActive_{false};
  float sampleRate_;
  float pitchBendRangeInSemitones_{7.f};
  float pitchGlideTimeInSeconds_{0.f};
  float pitchDriftAmount_{0.f};
  bool unison_{false};
  uint32_t currentNoteOnIndex{0};
  bool awake_{false};

  void dumpVoices();
  int testCounter{0};
};


inline std::ostream& operator<<(std::ostream& out, const Event& e)
{
  std::cout << "[" << e.type << "/" << e.channel << "/" << e.keyNumber << "/" << e.time << "]";
  return out;
}


}  // namespace ml
