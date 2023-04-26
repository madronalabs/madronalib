// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <deque>

#include "madronalib.h"
#include "mldsp.h"

namespace ml
{

// EventsToSignals processes different types of events and generates bundles of signals to
// control synthesizers.
//
class EventsToSignals final
{
public:

  static constexpr int kMaxVoices{16};
  static constexpr int kMaxEventsPerVector{128};
  
  static constexpr float kPitchGlideTimeSeconds{0.f};
  static constexpr float kGlideTimeSeconds{0.02f};
  
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
  
  // Event: something that happens.
  //
  struct Event
  {
    enum Type
    {
      kNull = 0,
      kNoteOn,
      kNoteRetrig,
      kNoteSustain,
      kNoteOff,
      kSustainPedal, // when sustain pedal is held, key releases generate kNoteSustain events
      kController,
      kPitchWheel,
      kNotePressure,
      kProgramChange
    };
    
    Event() = default;
    ~Event() = default;

    Type type{kNull};
    int channel;  
    int creatorID; // the MIDI key or touch number that created the event.
    int time; // time in samples from start of current process buffer
    float value1{0};
    float value2{0};
    float value3{0};
    float value4{0};
    
    explicit operator bool() const { return type != kNull; }
  };
  
  #pragma mark -
  
  // Voice: a voice that can play.
  //
  struct Voice
  {
  public:
    
    enum State
    {
      kOff,
      kOn,
      kSustain
    };
    
    Voice() = default;
    ~Voice() = default;

    void setSampleRate(float sr);

    void reset();

    // send to start processing a new buffer.
    void beginProcess();
    
    // send a note on, off update or sustain event to the voice.
    void writeNoteEvent(const Event& e, const Scale& Scale);

    // write all current info to the end of the current buffer.
    // add pitchBend to pitch.
    void endProcess(float pitchBend);
    
    int state{kOff};
    size_t nextFrameToProcess{0};

    // instantaneous values, written during event processing
    float currentVelocity{0};
    float currentPitch{0};
    float currentPitchBend{0};
    float currentMod{0};
    float currentX{0};
    float currentY{0};
    float currentZ{0};

    int creatorID{0}; // for matching event sources, could be MIDI key, or touch number.
    uint32_t ageInSamples{0}; 

    LinearGlide pitchGlide;
    LinearGlide pitchBendGlide;
    LinearGlide modGlide;
    LinearGlide xGlide;
    LinearGlide yGlide;
    LinearGlide zGlide;
    
    // output signals (velocity, pitch, voice... )
    DSPVectorArray< kNumVoiceOutputRows > outputs;
    
    float _sampleRate;
  };
  
  #pragma mark -
  EventsToSignals(int sr);
  ~EventsToSignals();
  
  size_t setPolyphony(int n);
  size_t getPolyphony();

  // clear all voices and reset state.
  void reset();

  // clear all events in queue.
  void clearEvents();

  // add an event to the queue.
  void addEvent(const Event& e);
  
  // process all events in queue and generate output signals.
  void process();
  
  void setPitchBendInSemitones(float f);

  // voices, containing signals for clients to read directly.
  std::vector< Voice > voices;
  
private:
  
  void processEvent(const Event &eventParam);
  void processNoteOnEvent(const Event& event);
  void processNoteOffEvent(const Event& event);
  void processNoteUpdateEvent(const Event& event);
  void processControllerEvent(const Event& event);
  void processPitchWheelEvent(const Event& event);
  void processNotePressureEvent(const Event& event);
  void processChannelPressureEvent(const Event& event);
  void processSustainEvent(const Event& event);

  // find a free voice in the range of voice indices (startVoice, startVoice + len).
  // if no free voice is found return -1.
  int findFreeVoice(size_t len);
  int findVoiceToSteal(Event e);
  int findNearestVoice(int note);
  
  void dumpVoices();
  
  // data
  Scale _scale;
  Queue< Event > _eventQueue;
  int _polyphony{0};
  int _voiceRotateOffset{0};
  bool _sustainPedalActive{false};
  float _sampleRate;
  float kPitchBendSemitones{7.f};
};


}  // namespace ml
