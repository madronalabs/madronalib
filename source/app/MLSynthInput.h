// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <deque>

#include "madronalib.h"
#include "mldsp.h"

namespace ml
{

// SynthInput processes different types of events and generates bundles of signals to
// control synthesizers.
//
class SynthInput final
{
public:
  
  static constexpr int kMaxVoices{16};
  static constexpr int kMaxEvents{1024};
  static constexpr int kMPEInputChannels{16};

  // rows per voice output signal.
  enum VoiceOutputSignals
  {
    kVelocity = 0,
    kPitch,
    kVoice,
    kAftertouch,
    kMod,
    kX,
    kY,
    kElapsedTime,
    kNumVoiceOutputRows
  };
  
  enum Protocol
  {
    kInputProtocolMIDI = 0,
    kInputProtocolMIDI_MPE
  };
  
  // Event: something that happens.
  //
  struct Event
  {
    enum Type
    {
      kNull = 0,
      kNoteOn,
      kNoteOff,
      kNoteUpdate, // OSC messages can update all controllers at once
      kNoteSustain, // when sustain pedal is held, key releases generate sustain events
      kController,
      kPitchWheel,
      kNotePressure,
      kChannelPressure,
      kProgramChange,
      kSustainPedal
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
    
    // send a note on, off or sustain event to the voice.
    void addNoteEvent(const Event& e, const Scale& Scale);
    void stealNoteEvent(const Event& e, const Scale& Scale, bool retrig);

    // write all current info to the end of the current buffer.
    void endProcess();

    
    int state{kOff};
    size_t nextTime{0};
    float velocity{0};
    float pitch{0};
    int creatorID{0}; // for matching event sources, could be MIDI key, or touch number.
    int age{0};  // time active, measured to the end of the current process buffer

    

    LinearGlide pitchGlide;
    LinearGlide aftertouchGlide;
    LinearGlide modGlide;
    LinearGlide xGlide;
    LinearGlide yGlide;
    
    
    // todo drift
    
    DSPVectorArray< kNumVoiceOutputRows > outputs;
    
    Event currentUnisonNoteEvent;
  };
  
  #pragma mark -
  SynthInput(int sr);
  ~SynthInput();
  
  size_t setPolyphony(int n);
  
  // clear all voices and reset state.
  void reset();

  // clear all events in queue.
  void clearEvents();

  // add an event to the queue.
  void addEvent(const Event& e);
  
  // process all events in queue and generate output signals.
  void processEvents();
  
private:
  
  void processEvent(const Event &eventParam);
  void doNoteOn(const Event& event);
  void doNoteOff(const Event& event);
  void doNoteUpdate(const Event& event);
  void doController(const Event& event);
  void doPitchWheel(const Event& event);
  void doNotePressure(const Event& event);
  void doChannelPressure(const Event& event);
  void doSustain(const Event& event);
  
  void writeOutputSignals();

  // find a free voice in the range of voice indices (startVoice, startVoice + len).
  // if no free voice is found return -1.
  int findFreeVoice(size_t startVoice, size_t len);
  
  int findOldestSustainedVoice();

  int findNearestVoice(int note);
  
  
  void dumpEvents();
  void dumpVoices();
  void dumpSignals();

  // the usual voices for each channel
  std::vector< Voice > _voices;

  Scale _scale;
  Protocol _protocol;
  
  Queue< Event > _eventQueue;
  
  // a special voice for the MPE "Main Channel"
  // stores main pitch bend and controller inputs, which are added to other voices.
  Voice _MPEMainVoice;
    
  std::array<LinearGlide, kMPEInputChannels + 1> _MPEPitchBendGlides;
  std::array<DSPVector, kMPEInputChannels + 1> _MPEPitchBendSignals;

  int _polyphony{0};

  int _nextEventIdx{0};
  int _voiceRotateOffset{0};
  int _eventTimeOffset{0};
  
  int _controllerNumber{-1};
  int _controllerMPEXNumber{73};
  
  int _currentVoices{0};
  int _driftCounter{-1};
  int _eventCounter{0};
  int _frameCounter{0};

  bool _glissando{false};
  bool _unisonMode{false};
  bool _rotateMode{true};
  int _unisonInputTouch{-1};
  float _unisonVel{0};
  float _glide{0};
  
  float _unisonPitch1;
  
  /* MPE Main Voice should take care of these?
  MLSignal _tempSignal;
  MLSignal _mainPitchSignal;
  MLSignal _mainChannelPressureSignal;
  MLSignal _mainModSignal;
  MLSignal _mainMod2Signal;
  MLSignal _mainMod3Signal;
  */
  
  float _pitchWheelSemitones{7};
  float _pitchWheelSemitonesMPE{12};

  
  float _masterTune {440.f};
  float _masterPitchOffset {0.f};
  
  bool _sustainPedalActive{false};
  
  ml::NoiseGen _rand;
  
  int _nullFrameCounter{0};

};

inline bool isFree(const SynthInput::Event& e)
{
  return bool(e);
}



inline int findFreeEvent(const std::deque< SynthInput::Event >& events)
{
  
  /*
  int r = -1;
  for(int i=0; i<size(); ++i)
  {
    if (at(i).isFree())
    {
      r = i;
      break;
    }
  }
  return r;
   */
  
  // TSET
  
  return 1;
}


}  // namespace ml
