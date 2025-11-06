// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "mldsp.h"
#include "MLSymbol.h"
#include "MLEvent.h"

namespace ml
{

// rows per voice output signal.
enum VoiceOutputSignals
{
  kPitch = 0,
  kGate,
  kVoice,
  kZ,
  kX,
  kY,
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
    kSustained
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
  static constexpr size_t kMaxEventsPerProcessBuffer{128};
  static constexpr size_t kMaxPhysicalKeys{128};
  static constexpr size_t kNumControllers{129};
  static constexpr int kChannelPressureControllerIdx{128};

  static constexpr float kGlideTimeSeconds{0.02f};
  static constexpr float kControllerGlideTimeSeconds{0.02f};
  static constexpr float kDriftTimeSeconds{8.0f};
  static constexpr float kDriftScale{0.02f};

  explicit EventsToSignals();
  ~EventsToSignals();

  void setSampleRate(double r);

  size_t setPolyphony(size_t n);
  size_t getPolyphony();

  // clear all voices and queued events and reset state.
  void clear();

  // just reset time outputs
  void resetTimes();

  // inserts an event to the buffer sorted by time.
  void addEvent(const Event& e);

  void clearEvents();

  // process incoming events in buffer and generate output signals.
  // events in the queue in the time range [startOffset, startOffset + kFloatsPerDSPVector) will
  // be processed. it is assumed that all events in the queue are sorted by start time. Any
  // events outside the time range will be ignored.
  void processVector(int startOffset);

  void setPitchBendInSemitones(float f);
  void setMPEPitchBendInSemitones(float f);
  void setPitchGlideInSeconds(float f);
  void setDriftAmount(float f);
  void setUnison(bool b);
  void setProtocol(Symbol p)
  {
    protocol_ = p;
    clear();
  }
  void setModCC(int c) { voiceModCC_ = c; }

#pragma mark -

  // Voice: a voice that can play.
  //
  struct Voice
  {
    Voice() = default;
    ~Voice() = default;

    void setSampleRate(double r);
    void setPitchGlideInSeconds(float g);
    void setDriftAmount(float d);

    void reset();

    void resetTime();

    // send to start processing a new buffer.
    void beginProcess();

    // send a note on, off update or sustain event to the voice.
    void writeNoteEvent(const Event& e, int keyIdx, bool doGlide, bool doReset);

    // write all current info to the end of the current buffer, scaling pitch bend
    void endProcess(float pitchBend);

    // data

    // output signals (velocity, pitch, voice... )
    DSPVectorArray<kNumVoiceOutputRows> outputs;

    size_t nextFrameToProcess{0};

    // instantaneous values, written during event processing
    float currentVelocity{0};
    float currentPitch{0};
    float currentPitchBend{0};
    float currentMod{0};
    float currentX{0};
    float currentY{0};
    float currentZ{0};

    // physical key or touch # of creator. 0 = undefined.
    size_t creatorKeyIdx_{0};
    uint32_t eventAgeInSamples{0};

    // amount to increase event age each sample—either 0 or 1
    uint32_t eventAgeStep{0};

    // pitch glide
    SampleAccurateLinearGlide pitchGlide;
    LinearGlide pitchBendGlide;
    LinearGlide modGlide;
    LinearGlide xGlide;
    LinearGlide yGlide;
    LinearGlide zGlide;
    float pitchGlideTimeInSeconds{0};
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

    double sr{0};
    double isr{0};
    int voiceIndex{0};
    bool recalcNeeded{false};
  };

  struct SmoothedController
  {
    void setSampleRate(double r);
    void process();

    LinearGlide glide;
    DSPVector output{0.f};
    float inputValue{0.f};
    double sr{0};
    bool recalcNeeded{false};
  };

  // get a const reference to a Voice for reading its output.
  // Lifetime management is an issue though: don't hang onto this reference!
  const Voice& getVoice(int n) const { return voices[n + 1]; }

  // get voice which had a note on event most recently, if any
  int getNewestVoice() const { return newestVoice_ - 1; }

  const SmoothedController& getController(size_t n) const { return controllers[n]; }

 private:
  size_t countHeldNotes();
  void processEvent(const Event& eventParam);
  void processNoteOnEvent(const Event& event);
  void processNoteOffEvent(const Event& event);
  void processNoteUpdateEvent(const Event& event);
  void processControllerEvent(const Event& event);
  void processPitchWheelEvent(const Event& event);
  void processNotePressureEvent(const Event& event);
  void processChannelPressureEvent(const Event& event);
  void processSustainPedalEvent(const Event& event);
  int findFreeVoice();
  int findVoiceToSteal(Event e);
  int findNearestVoice(int note);

  // voices, containing signals for clients to read directly.
  // voices[0] is the "main voice" used for MPE.
  std::vector<Voice> voices;

  // output values for continuous controllers.
  std::vector<SmoothedController> controllers;

  Symbol protocol_{"MIDI"};

  // set a special modulation # to send out in each voice
  int voiceModCC_{16};

  std::array<KeyState, kMaxPhysicalKeys> keyStates_;
  std::vector<Event> eventBuffer_;

  size_t polyphony_{0};
  int lastFreeVoiceFound_{-1};
  int newestVoice_{-1};
  bool sustainPedalActive_{false};
  double sr{0};
  float pitchBendRangeInSemitones_{7.f};
  float mpePitchBendRangeInSemitones_{24.f};
  bool unison_{false};
  uint32_t currentNoteOnIndex{0};
  bool awake_{false};

  void dumpVoices();
  int testCounter{0};
};

}  // namespace ml
