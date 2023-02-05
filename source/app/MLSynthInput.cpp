// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/



#include "MLSynthInput.h"

namespace ml {

const ml::Symbol voicesSym("voices");
const ml::Symbol data_rateSym("data_rate");
const ml::Symbol scaleSym("scale");
const ml::Symbol protocolSym("protocol");
const ml::Symbol bendSym("bend");
const ml::Symbol bendMPESym("bend_mpe");
const ml::Symbol modSym("mod");
const ml::Symbol modMPEXSym("mod_mpe_x");
const ml::Symbol unisonSym("unison");
const ml::Symbol glideSym("glide");

const int kMaxEvents = 1 << 4;//1 << 8; // max events per signal vector
static const int kNumVoiceSignals = 8;

const ml::Symbol voiceSignalNames[kNumVoiceSignals]
{
  "pitch",
  "gate",
  "vel",
  "voice",
  "after",
  "moda",
  "modb",
  "modc"
};

#if INPUT_DRIFT
const float kDriftConstants[16] =
{
  0.465f, 0.005f, 0.013f, 0.019f,
  0.155f, 0.933f, 0.002f, 0.024f,
  0.943f, 0.924f, 0.139f, 0.501f,
  0.196f, 0.591f, 0.961f, 0.442f
};
const float SynthInput::kDriftConstantsAmount = 0.004f;
const float SynthInput::kDriftRandomAmount = 0.002f;
#endif


#pragma mark -
// Voice
//

static const float kDriftIntervalSeconds{0.5f};
static const float kGlideTimeSeconds{0.5f};


void SynthInput::Voice::setSampleRate(float sr)
{
  pitchGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  aftertouchGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  modGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  xGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  yGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
}


// done when DSP is reset.
void SynthInput::Voice::reset()
{
  state = kOff;
  nextTime = 0;
  age = 0;
  velocity = 0;
  pitch = 0;
  creatorID = 0;
}

void SynthInput::Voice::beginProcess()
{
  nextTime = 0;
}


// TODO retrig if needed

// write current pitch and vel to next note time


// set new note pitch, vel, creatorID


// start new note, setting note and velocity from OSC or MIDI

void SynthInput::Voice::addNoteEvent(const Event& e, const Scale& scale)
{
  auto time = e.time;
  
  switch(e.type)
  {
    case Event::kNoteOn:
    {
      state = kOn;
      age = 0;
      size_t destTime = e.time;
      destTime = clamp(destTime, 0UL, (size_t)kFloatsPerDSPVector);

      // write current pitch and vel up to note start
      for(size_t t = nextTime; t < destTime; ++t)
      {
        outputs[kVelocity] = velocity;
        
        // TODO sample accurate glide
        outputs[kPitch] = pitch;
      }
       
      // set new values
      pitch = scale.noteToLogPitch(e.value1);
      velocity = e.value2; // TODO union

      break;
    }
    case Event::kNoteUpdate:
      // update note, z, x, y from OSC
      
      // update note and z
      //mdPitch.addChange(scale.noteToLogPitch(e.value1), time);
      //mdAmp.addChange(e.value2, time);
      
      // KEY outputs: dy, x, y
      // TODO add UI / settings for relative or absolute options
      //mdMod.addChange(e.mValue4 - mStartY, time);
      
      //mdMod2.addChange(e.value3*2.f - 1.f, time);
      //mdMod3.addChange(e.mValue4*2.f - 1.f, time);
      break;
      
    case Event::kNoteSustain:
      // sent when note is released iwth sustain pedal on
      // no signal changes, but changes state to sustain
      state = kSustain;
      break;
    case Event::kNoteOff:
    {
      state = kOff;

      size_t destTime = e.time;
      destTime = clamp(destTime, 0UL, (size_t)kFloatsPerDSPVector);
      
      // write current values up to change
      for(size_t t = nextTime; t < destTime; ++t)
      {
        outputs[kVelocity] = velocity;
        
        // TODO sample accurate glide
        outputs[kPitch] = pitch;
      }
      
      // set new values
      velocity = 0.;
      
      break;
    }
    default:
      state = kOff;
      
      //  std::cout << "add note event OFF: inst: " << e.creatorID << " pitch " << mNote << "\n";
      // mdGate.addChange(0.f, time);
      //  mdAmp.addChange(0.f, time);
      //  mdVel.addChange(0.f, time);
      //   mdNotePressure.addChange(0.f, time);
      
      // for MPE mode when controlling envelopes with aftertouch: ensure
      // notes are not sending pressure when off
      //  mdChannelPressure.addChange(0.f, time);
      age = 0;
      
      // we leave channel alone so that pitch bends will retain their values when the note ends
      
      break;
  }
  
  
  
  currentUnisonNoteEvent = e;
}


void SynthInput::Voice::stealNoteEvent(const Event& e, const Scale& Scale, bool retrig)
{
  float note = e.value1;
  float vel = e.value2;
  auto time = e.time;
  if (time == 0) time++; // in case where time = 0, make room for retrigger.
  
  state = kOn;
//  creatorID = e.creatorID;
  note = note;
  age = 0;
  // mdPitch.addChange(scale.noteToLogPitch(note), time);
  
  if (retrig)
  {
    //   mdGate.addChange(0.f, time - 1);
    //   mdNotePressure.addChange(0.f, time - 1);
    
  }
  
  //  mdGate.addChange(1, time);
  //  mdAmp.addChange(vel, time);
  //  mdVel.addChange(vel, time);
  
  currentUnisonNoteEvent = e;
  
}


void SynthInput::Voice::endProcess()
{
  
  for(size_t t = nextTime; t < kFloatsPerDSPVector; ++t)
  {
    outputs[kVelocity] = velocity;
    
    // write pitch to end of buffer.
    // TODO sample accurate glide
    outputs[kPitch] = pitch;

  }
}


#pragma mark -

// registry section

/*
 namespace
 {
 MLProcRegistryEntry<SynthInput> classReg("midi_to_signals");
 ML_UNUSED MLProcParam<SynthInput> params[12] = { "bufsize", "voices", "bend", "bend_mpe", "mod", "mod_mpe_x", "unison", "glide", "protocol", "data_rate" , "scale", "master_tune"};
 // no input signals.
 ML_UNUSED MLProcOutput<SynthInput> outputs[] = {"*"};  // variable outputs
 }
 */

SynthInput::SynthInput(int sr) : _eventQueue(kMaxEvents)
{
  _voices.resize(kMaxVoices);
  
  for(int i=0; i<kMaxVoices; ++i)
  {
    _voices[i].setSampleRate(sr);
  }
}

SynthInput::~SynthInput()
{
}

size_t SynthInput::setPolyphony(int n)
{
  _polyphony = std::min(n, kMaxVoices);
  return _polyphony;
}

/*
 // set up output buffers
 MLProc::err SynthInput::resize()
 {
 float sr = getContextSampleRate();
 static const ml::Symbol bufsizeSym("bufsize");
 
 MLProc::err re = OK;
 
 // resize voices
 //
 int bufSize = (int)getParam(bufsizeSym);
 int vecSize = getContextVectorSize();
 
 int kMaxVoices = getContext()->getRootContext()->getMaxVoices();
 _voices.resize(kMaxVoices);
 
 MLProc::err r;
 for(int i=0; i<kMaxVoices; ++i)
 {
 _voices[i].setSampleRate(sr);
 _voices[i].resize(bufSize);
 }
 _MPEMainVoice.resize(bufSize);
 
 // make signals that apply to all voices
 mTempSignal.setDims(vecSize);
 mMainPitchSignal.setDims(vecSize);
 mMainChannelPressureSignal.setDims(vecSize);
 mMainModSignal.setDims(vecSize);
 mMainMod2Signal.setDims(vecSize);
 mMainMod3Signal.setDims(vecSize);
 
 // make outputs
 //
 for(int i=1; i <= kMaxVoices * kNumVoiceSignals; ++i)
 {
 if (!outputIsValid(i))
 {
 setOutput(i, getContext()->getNullOutput());
 }
 }
 
 // do voice params
 //
 for(int i=0; i<kMaxVoices; ++i)
 {
 if((i*kNumVoiceSignals + 1) < getNumOutputs())
 {
 // set initial pitch to 0.
 _voices[i].mdPitch.addChange(0.f, 0);
 MLSignal& out = getOutput(i*kNumVoiceSignals + 1);
 _voices[i].mdPitch.writeToSignal(out, vecSize);
 _voices[i].mdPitchBend.addChange(0.f, 0);
 _voices[i].mdDrift.setGlideTime(kDriftInterval);
 }
 }
 
 for(auto& c : mPitchBendChangesByChannel)
 {
 c.setSampleRate(sr);
 c.setDims(vecSize);
 }
 for(auto& c : mPitchBendSignals)
 {
 c.setDims(vecSize);
 }
 
 clearChangeLists();
 return re;
 }
 */

/*
 
 void SynthInput::doParams()
 {
 int kMaxVoices = getContext()->getRootContext()->getMaxVoices();
 int newVoices = (int)getParam(voicesSym);
 newVoices = ml::clamp(newVoices, 0, 15);
 
 // TODO enable / disable voice containers here
 mOSCDataRate = (int)getParam(data_rateSym);
 
 const ml::Text& scaleName = getTextParam(scaleSym);
 _scale.loadFromRelativePath(scaleName);
 
 mMasterTune = getParam("master_tune");
 if(within(mMasterTune, 220.f, 880.f))
 {
 mMasterPitchOffset = log2f(mMasterTune/440.f);
 }
 
 const int newProtocol = (int)getParam(protocolSym);
 _protocol = newProtocol;
 
 mGlide = getParam(glideSym);
 for (int v=0; v<kMaxVoices; ++v)
 {
 _voices[v].mdPitch.setGlideTime(mGlide);
 _voices[v].mdPitchBend.setGlideTime(mGlide);
 }
 _MPEMainVoice.mdPitchBend.setGlideTime(mGlide);
 
 float oscGlide = (1.f / std::max(100.f, (float)mOSCDataRate));
 
 switch(_protocol)
 {
 case kInputProtocolOSC:
 for(int i=0; i<kMaxVoices; ++i)
 {
 _voices[i].mdGate.setGlideTime(0.0f);
 _voices[i].mdAmp.setGlideTime(oscGlide);
 _voices[i].mdVel.setGlideTime(0.0f);
 _voices[i].mdNotePressure.setGlideTime(oscGlide);
 _voices[i].mdChannelPressure.setGlideTime(oscGlide);
 _voices[i].mdMod.setGlideTime(oscGlide);
 _voices[i].mdMod2.setGlideTime(oscGlide);
 _voices[i].mdMod3.setGlideTime(oscGlide);
 }
 break;
 case kInputProtocolMIDI:
 case kInputProtocolMIDI_MPE:
 for(int i=0; i<kMaxVoices; ++i)
 {
 _voices[i].mdGate.setGlideTime(0.f);
 _voices[i].mdAmp.setGlideTime(0.001f);
 _voices[i].mdVel.setGlideTime(0.f);
 _voices[i].mdNotePressure.setGlideTime(0.001f);
 _voices[i].mdChannelPressure.setGlideTime(0.001f);
 _voices[i].mdMod.setGlideTime(0.001f);
 _voices[i].mdMod2.setGlideTime(0.001f);
 _voices[i].mdMod3.setGlideTime(0.001f);
 }
 break;
 }
 
 if (newVoices != _polyphony)
 {
 _polyphony = newVoices;
 clear();
 }
 
 // pitch wheel mult
 _pitchWheelSemitones = getParam(bendSym);
 
 // pitch wheel mult
 _pitchWheelSemitonesMPE = getParam(bendMPESym);
 
 // std::cout << "SynthInput: master bend " << _pitchWheelSemitones << ", MPE bend " << _pitchWheelSemitonesMPE << "\n";
 
 // listen to controller number mods
 _controllerNumber = (int)getParam(modSym);
 _controllerMPEXNumber = (int)getParam(modMPEXSym);
 
 int unison = (int)getParam(unisonSym);
 if (mUnisonMode != unison)
 {
 mUnisonMode = unison;
 clear();
 }
 
 mParamsChanged = false;
 //dumpParams();  // DEBUG
 }
 */

void SynthInput::reset()
{
  _eventQueue.clear();
  
  for(auto& v : _voices)
  {
    v.reset();
  }
  
  // clear glides in progress
  
  // set all other signal generators to defaults
  
  /*
   for (int v=0; v<kMaxVoices; ++v)
   {
   _voices[v].clearState();
   _voices[v].clearChanges();
   //      _voices[v].zeroExceptPitch();
   _voices[v].zeroPressure();
   
   MLSignal& pitch = getOutput(v*kNumVoiceSignals + 1);
   MLSignal& gate = getOutput(v*kNumVoiceSignals + 2);
   MLSignal& velSig = getOutput(v*kNumVoiceSignals + 3);
   MLSignal& voiceSig = getOutput(v*kNumVoiceSignals + 4);
   MLSignal& after = getOutput(v*kNumVoiceSignals + 5);
   MLSignal& mod = getOutput(v*kNumVoiceSignals + 6);
   MLSignal& mod2 = getOutput(v*kNumVoiceSignals + 7);
   MLSignal& mod3 = getOutput(v*kNumVoiceSignals + 8);
   
   _voices[v].mdPitch.writeToSignal(pitch, vecSize);
   _voices[v].mdGate.writeToSignal(gate, vecSize);
   _voices[v].mdVel.writeToSignal(velSig, vecSize);
   voiceSig.setToConstant(v);
   
   _voices[v].mdNotePressure.writeToSignal(after, vecSize);
   _voices[v].mdChannelPressure.writeToSignal(after, vecSize);
   _voices[v].mdAmp.writeToSignal(after, vecSize);
   
   _voices[v].mdMod.writeToSignal(mod, vecSize);
   _voices[v].mdMod2.writeToSignal(mod2, vecSize);
   _voices[v].mdMod3.writeToSignal(mod3, vecSize);
   }
   */
  
  /*
   _MPEMainVoice.clearState();
   _MPEMainVoice.clearChanges();
   //    _MPEMainVoice.zeroExceptPitch();
   _MPEMainVoice.zeroPressure();
   */
  
  _eventCounter = 0;
}


// order of signals:
// pitch
// gate
// amp (gate * velocity)
// vel (velocity, stays same after note off)
// voice
// aftertouch
// mod, mod2, mod3

// display MIDI: pitch gate vel voice after mod -2 -3 -4
// display OSC: pitch gate vel(constant during hold) voice(touch) after(z) dx dy x y

/*
 void SynthInput::process()
 {
 if (mParamsChanged) doParams();
 int sr = getContextSampleRate();
 clearChangeLists();
 
 #if INPUT_DRIFT
 // update drift change list for each voice
 if ((mDriftCounter < 0) || (mDriftCounter > sr*kDriftInterval))
 {
 for (int v=0; v<_polyphony; ++v)
 {
 float drift = (kDriftConstants[v] * kDriftConstantsAmount) + (mRand.getSample()*kDriftRandomAmount);
 _voices[v].mdDrift.addChange(drift, 1);
 }
 mDriftCounter = 0;
 }
 mDriftCounter += kFloatsPerDSPVector;
 #endif
 
 // update age for each voice
 for (int v=0; v<_polyphony; ++v)
 {
 if(_voices[v].mAge >= 0)
 {
 _voices[v].mAge += kFloatsPerDSPVector;
 }
 }
 
 // generate change lists from events
 
 
 processEvents();
 
 
 // generate output signals from change lists
 writeOutputSignals(kFloatsPerDSPVector);
 
 mFrameCounter += kFloatsPerDSPVector;
 if(mFrameCounter > sr)
 {
 //dumpEvents();
 //dumpVoices();
 //dumpSignals();
 //dumpTouchFrame();
 mFrameCounter -= sr;
 }
 }*/

void SynthInput::addEvent(const Event& e)
{
  _eventQueue.push(e);
}

void SynthInput::processEvents()
{
  for(auto& v : _voices)
  {
    v.beginProcess();
  }
  while(Event e = _eventQueue.pop())
  {
    processEvent(e);
  }
  
  for(auto& v : _voices)
  {
    v.endProcess();
  }
}


// process one incoming event by making the appropriate changes in state and change lists.
void SynthInput::processEvent(const Event &eventParam)
{
  Event event = eventParam;
  
  switch(event.type)
  {
    case Event::kNoteOn:
      doNoteOn(event);
      break;
    case Event::kNoteOff:
      doNoteOff(event);
      break;
    case Event::kNoteUpdate:
      doNoteUpdate(event);
      break;
    case Event::kController:
      doController(event);
      break;
    case Event::kPitchWheel:
      doPitchWheel(event);
      break;
    case Event::kNotePressure:
      doNotePressure(event);
      break;
    case Event::kChannelPressure:
      doChannelPressure(event);
      break;
    case Event::kSustainPedal:
      doSustain(event);
      break;
      /*
       case Event::kPitchBendRange:
       doPitchBendRange(event);
       break;
       */
    case Event::kNull:
    default:
      break;
  }
}

void SynthInput::doNoteOn(const Event& e)
{
  
  std::cout << " SynthInput::doNoteOn " << e.time << " " << e.value1 << " " << e.value2 << "\n";
  return;
  
  /*
   if(mUnisonMode)
   {
   // push any event previously occupying voices to pending stack
   // assuming all voices are playing the same event.
   if (_voices[0].state == Voice::kOn)
   {
   const Event& prevEvent = _voices[0].mCurrentUnisonNoteEvent;
   mNoteEventsPending.push(prevEvent);
   mNoteEventsPlaying.clearEventsMatchingID(prevEvent.creatorID);
   }
   for (int v = 0; v < _polyphony; ++v)
   {
   _voices[v].addNoteEvent(event, _scale);
   }
   }
   
   else
   */
  
  {
    auto v = findFreeVoice(0, _polyphony);
    if(v >= 0)
    {
      _voiceRotateOffset++;
      _voices[v].addNoteEvent(e, _scale);
    }
    else
    {
      // find a sustained voice to steal
      v = findOldestSustainedVoice();
      
      // or failing that, the voice with the nearest note
      if(v < 0)
      {
        int note = e.value1;
        v = findNearestVoice(note);
      }
      
      // steal it with retrigger
      _voices[v].stealNoteEvent(e, _scale, true);
    }
  }
}

void SynthInput::doNoteOff(const Event& e)
{
  
  std::cout << " SynthInput::doNoteOff " << e.time << " " << e.value1 << " " << e.value2 << "\n";
  return;
  
  // clear all events matching creatoe
  int creator = e.creatorID;
  int chan = e.channel;
  for (int i=0; i<kMaxEvents; ++i)
  {
    //if(_eventsPlaying[i].creatorID == creator)
    {
      //_eventsPlaying[i].clear();
    }
  }
  
  /*
   if(_unisonMode)
   {
   // if note off is the sounding event,
   // play the most recent note from pending stack, or release or sustain last note.
   // else delete the note from events and pending stack.
   if(_voices[0].creatorID == creator)
   {
   if(!mNoteEventsPending.isEmpty())
   {
   Event pendingEvent = mNoteEventsPending.pop();
   for (int v = 0; v < _polyphony; ++v)
   {
   _voices[v].stealNoteEvent(pendingEvent, _scale, mGlissando);
   }
   }
   else
   {
   // release or sustain
   Event::EventType newEventType = _sustainPedalActive ? Event::kNoteSustain : Event::kNoteOff;
   for(int v=0; v<_polyphony; ++v)
   {
   Voice& voice = _voices[v];
   Event eventToSend = event;
   eventToSend.type = newEventType;
   voice.addNoteEvent(eventToSend, _scale);
   }
   }
   }
   else
   {
   mNoteEventsPending.clearEventsMatchingID(creatoe);
   }
   }
   else
   */
  
  {
    // send either off or sustain event to voices matching creatoe
    Event::Type newEventType = _sustainPedalActive ? Event::kNoteSustain : Event::kNoteOff;
    
    for(int v = 0; v < _polyphony; ++v)
    {
      Voice& voice = _voices[v];
      if((voice.creatorID == creator) && (voice.state == Voice::kOn))
      {
        Event eventToSend = e;
        eventToSend.type = newEventType;
        voice.addNoteEvent(eventToSend, _scale);
      }
    }
  }
}

// update multiple axes of control for a held note event.
void SynthInput::doNoteUpdate(const Event& event)
{
  int creator = event.creatorID;
  for(int v = 0; v < _polyphony; ++v)
  {
    Voice& voice = _voices[v];
    if((voice.creatorID == creator) && (voice.state == Voice::kOn))
    {
      _voices[v].addNoteEvent(event, _scale);
    }
  }
}


// if the controller number matches one of the numbers we are sending to the
// patcher, update it.
void SynthInput::doController(const Event& event)
{
  int creatoe = event.creatorID;
  auto time = event.time;
  int ctrl = event.value1;
  int chan = event.channel;
  float val = event.value2;
  
  switch(_protocol)
  {
      // note: this is for MIDI. OSC controller changes are handled through doNoteUpdate()
    case kInputProtocolMIDI:
    {
      if(ctrl == 120)
      {
        if(val == 0)
        {
          // all sound off
          reset();
        }
      }
      else if(ctrl == 123)
      {
        if(val == 0)
        {
          // all notes off
          for(int v=0; v<_polyphony; ++v)
          {
            Voice& voice = _voices[v];
            if(voice.state != Voice::kOff)
            {
              Event eventToSend = event;
              eventToSend.type = Event::kNoteOff;
              voice.addNoteEvent(eventToSend, _scale);
            }
          }
        }
      }
      else
      {
        // modulate all voices.
        for (int i=0; i<_polyphony; ++i)
        {/*
          if(ctrl == _controllerNumber)
          _voices[i].mdMod.addChange(val, time);
          else if (ctrl == _controllerNumber + 1)
          _voices[i].mdMod2.addChange(val, time);
          else if (ctrl == _controllerNumber + 2)
          _voices[i].mdMod3.addChange(val, time);
          */
        }
      }
      break;
    }
    case kInputProtocolMIDI_MPE:
    {
      if(chan == 1) // MPE main voice
      {
        if(ctrl == 120)
        {
          if(val == 0)
          {
            // all sound off
            reset();
          }
        }
        else if(ctrl == 123)
        {
          if(val == 0)
          {
            // all notes off
            for(int v=0; v<_polyphony; ++v)
            {
              Voice& voice = _voices[v];
              if(voice.state != Voice::kOff)
              {
                Event eventToSend = event;
                eventToSend.type = Event::kNoteOff;
                voice.addNoteEvent(eventToSend, _scale);
              }
            }
          }
        }
        else
        {
          /*
           // TODO add parameter for x cc
           if (ctrl == _controllerMPEXNumber) // x for MPE input, default 73
           _MPEMainVoice.mdMod2.addChange(val, time);
           else if (ctrl == 74) // y always 74
           _MPEMainVoice.mdMod3.addChange(val, time);
           else if(ctrl == _controllerNumber)
           {
           _MPEMainVoice.mdMod.addChange(val, time);
           }*/
          
        }
      }
      else // modulate voices matching creatoe
      {
        for(int v=0; v<_polyphony; ++v)
        {
          Voice& voice = _voices[v];
        //  if((voice.creatorID == creatoe) && (voice.state == Voice::kOn))
          {
            /*
             if (ctrl == _controllerMPEXNumber) // x for MPE input, default 73
             _voices[v].mdMod2.addChange(val, time);
             else if (ctrl == 74) // y always 74
             _voices[v].mdMod3.addChange(val, time);
             else if(ctrl == _controllerNumber)
             _voices[v].mdMod.addChange(val, time);
             */
          }
        }
      }
      break;
    }
  }
}

void SynthInput::doPitchWheel(const Event& event)
{
  float val = event.value1;
  float center = val - 8192.f;
  float bendAmount = center / 8191.f;
  const float bendScale = _pitchWheelSemitones / 12.f;
  const float mpeBendScale = _pitchWheelSemitonesMPE / 12.f;
  int chan = event.channel; // JUCE event channels are in range [1, 16]
  
  switch(_protocol)
  {
    case kInputProtocolMIDI:
    {
      for (int i=0; i<_polyphony; ++i)
      {
        //_voices[i].mdPitchBend.addChange(bendAmount*bendScale, event.time);
      }
      break;
    }
    case kInputProtocolMIDI_MPE:
    {
      if (chan == 1)
      {
        // write to MPE Main Channel
        //_MPEMainVoice.mdPitchBend.addChange(bendAmount*bendScale, event.time);
      }
      else
      {
        // write to MPE voice channel
        if(within(chan, 2, kMPEInputChannels + 1))
        {
          // mPitchBendChangesByChannel[chan].addChange(bendAmount*mpeBendScale, event.time);
        }
      }
      break;
    }
  }
}

void SynthInput::doNotePressure(const Event& event)
{
  switch(_protocol)
  {
    case kInputProtocolMIDI:
    {
      for (int i=0; i<_polyphony; ++i)
      {
        if (event.creatorID == _voices[i].creatorID)
        {
          //_voices[i].mdNotePressure.addChange(event.value2, event.time);
        }
      }
      break;
    }
    case kInputProtocolMIDI_MPE:    // note pressure is ignored in MPE mode
    {
      break;
    }
  }
}

void SynthInput::doChannelPressure(const Event& event)
{
  switch(_protocol)
  {
    case kInputProtocolMIDI:
    {
      for (int i=0; i<_polyphony; ++i)
      {
        //_voices[i].mdChannelPressure.addChange(event.value1, event.time);
      }
      break;
    }
    case kInputProtocolMIDI_MPE:
    {
      if (event.channel == 1) // MPE Main Channel
      {
        //_MPEMainVoice.mdChannelPressure.addChange(event.value1, event.time);
      }
      else
      {
        // send pressure to all active voices matching creatoe
        for (int v=0; v<_polyphony; ++v)
        {
          if ((_voices[v].creatorID == event.creatorID) && (_voices[v].state == Voice::kOn))
          {
            //   _voices[v].mdChannelPressure.addChange(event.value1, event.time);
          }
        }
      }
      break;
    }
  }
}

void SynthInput::doSustain(const Event& event)
{
  _sustainPedalActive = (int)event.value1;
  if(!_sustainPedalActive)
  {
    // clear any sustaining voices
    for(int i=0; i<_polyphony; ++i)
    {
      Voice& v = _voices[i];
      if(v.state == Voice::kSustain)
      {
        Event newEvent;
        newEvent.type = Event::kNoteOff;
        v.addNoteEvent(newEvent, _scale);
      }
    }
  }
}


// process change lists to make output signals
//
void SynthInput::writeOutputSignals()
{
  /*
   // get main channel signals for MPE
   if(_protocol == kInputProtocolMIDI_MPE)
   {
   _MPEMainVoice.mdPitchBend.writeToSignal(mMainPitchSignal, frames);
   _MPEMainVoice.mdChannelPressure.writeToSignal(mMainChannelPressureSignal, frames);
   _MPEMainVoice.mdMod.writeToSignal(mMainModSignal, frames);
   _MPEMainVoice.mdMod2.writeToSignal(mMainMod2Signal, frames);
   _MPEMainVoice.mdMod3.writeToSignal(mMainMod3Signal, frames);
   }
   
   // get pitch bend input channel signals for MPE
   if(_protocol == kInputProtocolMIDI_MPE)
   {
   for(int i=2; i < kMPEInputChannels + 1; ++i)
   {
   auto& c = mPitchBendChangesByChannel[i];
   c.writeToSignal(mPitchBendSignals[i], frames);
   }
   }
   
   int kMaxVoices = getContext()->getRootContext()->getMaxVoices();
   for (int v=0; v<kMaxVoices; ++v)
   {
   // changes per voice
   MLSignal& pitch = getOutput(v*kNumVoiceSignals + 1);
   MLSignal& gate = getOutput(v*kNumVoiceSignals + 2);
   MLSignal& velSig = getOutput(v*kNumVoiceSignals + 3);
   MLSignal& voiceSig = getOutput(v*kNumVoiceSignals + 4);
   MLSignal& after = getOutput(v*kNumVoiceSignals + 5);
   MLSignal& mod = getOutput(v*kNumVoiceSignals + 6);
   MLSignal& mod2 = getOutput(v*kNumVoiceSignals + 7);
   MLSignal& mod3 = getOutput(v*kNumVoiceSignals + 8);
   
   // write signals
   if (v < _polyphony)
   {
   // write pitch
   _voices[v].mdPitch.writeToSignal(pitch, frames);
   
   // add pitch bend
   if(_protocol == kInputProtocolMIDI)
   {
   // add voice pitch bend in semitones to pitch
   _voices[v].mdPitchBend.writeToSignal(mTempSignal, frames);
   pitch.add(mTempSignal);
   }
   
   else if(_protocol == kInputProtocolMIDI_MPE)
   {
   // add pitch from MPE main voice
   pitch.add(mMainPitchSignal);
   
   if(within(_voices[v].channel, 2, kMPEInputChannels + 1))
   {
   pitch.add(mPitchBendSignals[_voices[v].channel]);
   }
   }
   #if INPUT_DRIFT
   // write to common temp drift signal, we add one change manually so read offset is 0
   _voices[v].mdDrift.writeToSignal(mTempSignal, frames);
   pitch.add(mTempSignal);
   #endif
   
   // write master_tune param offset
   mTempSignal.fill(mMasterPitchOffset);
   pitch.add(mTempSignal);
   
   _voices[v].mdGate.writeToSignal(gate, frames);
   
   // initial velocity output
   _voices[v].mdVel.writeToSignal(velSig, frames);
   
   // voice constant output
   voiceSig.setToConstant(v);
   
   // aftertouch / z output
   switch(_protocol)
   {
   case kInputProtocolMIDI:
   // add channel aftertouch + poly aftertouch.
   _voices[v].mdNotePressure.writeToSignal(after, frames);
   _voices[v].mdChannelPressure.writeToSignal(mTempSignal, frames);
   after.add(mTempSignal);
   break;
   case kInputProtocolMIDI_MPE:
   // MPE ignores poly aftertouch.
   _voices[v].mdChannelPressure.writeToSignal(after, frames);
   //after.add(mMainChannelPressureSignal);
   break;
   case kInputProtocolOSC:
   // write amplitude to aftertouch signal
   _voices[v].mdAmp.writeToSignal(after, frames);
   break;
   }
   
   _voices[v].mdMod.writeToSignal(mod, frames);
   _voices[v].mdMod2.writeToSignal(mod2, frames);
   _voices[v].mdMod3.writeToSignal(mod3, frames);
   
   if(_protocol == kInputProtocolMIDI_MPE)
   {
   mod.add(mMainModSignal);
   mod2.add(mMainMod2Signal);
   mod3.add(mMainMod3Signal);
   
   // over MPE, we can make bipolar x and y signals to match the OSC usage.
   // only center the x controller if the controller number being used is the default.
   if(_controllerMPEXNumber == 73)
   {
   mod2.scale(2.0f);
   mod2.add(-1.0f);
   }
   mod3.scale(2.0f);
   mod3.add(-1.0f);
   }
   
   // clear change lists
   _voices[v].mdPitch.clearChanges();
   _voices[v].mdPitchBend.clearChanges();
   _voices[v].mdGate.clearChanges();
   _voices[v].mdAmp.clearChanges();
   _voices[v].mdVel.clearChanges();
   _voices[v].mdNotePressure.clearChanges();
   _voices[v].mdChannelPressure.clearChanges();
   _voices[v].mdMod.clearChanges();
   _voices[v].mdMod2.clearChanges();
   _voices[v].mdMod3.clearChanges();
   #if INPUT_DRIFT
   _voices[v].mdDrift.clearChanges();
   #endif
   }
   else
   {
   pitch.setToConstant(0.f);
   gate.setToConstant(0.f);
   velSig.setToConstant(0.f);
   voiceSig.setToConstant(0.f);
   after.setToConstant(0.f);
   mod.setToConstant(0.f);
   mod2.setToConstant(0.f);
   mod3.setToConstant(0.f);
   }
   }*/
  
}

#pragma mark -

// return index of free voice or -1 for none.
// increments mVoiceRotateOffset.
//
int SynthInput::findFreeVoice(size_t start, size_t len)
{
  int r = -1;
  for (auto v = start; v < start + len; ++v)
  {
    auto vr = v;
    if(_rotateMode)
    {
      vr = (vr + _voiceRotateOffset) % len;
    }
    if (_voices[vr].state == Voice::kOff)
    {
      r = static_cast<int>(vr);
      break;
    }
  }
  return r;
}

int SynthInput::findOldestSustainedVoice()
{
  int r = -1;
  std::list<int> sustainedVoices;
  for (int i=0; i<_polyphony; ++i)
  {
    Voice& v = _voices[i];
    if(v.state == Voice::kSustain)
    {
      sustainedVoices.push_back(i);
    }
  }
  
  int maxAge = -1;
  for(std::list<int>::const_iterator it = sustainedVoices.begin();
      it != sustainedVoices.end(); it++)
  {
    int voiceIdx = *it;
    int age = _voices[voiceIdx].age;
    if (age > maxAge)
    {
      maxAge = age;
      r = voiceIdx;
    }
  }
  return r;
}

// return the index of the voice with the note nearest to the note n.
// Must always return a valid voice index.
int SynthInput::findNearestVoice(int note)
{
  int r = 0;
  int minDist = 128;
  /*
  for (int v=0; v<_polyphony; ++v)
  {
    int vNote = _voices[v].note;
    int noteDist = std::abs(note - vNote);
    if (noteDist < minDist)
    {
      minDist = noteDist;
      r = v;
    }
  }*/
  return r;
}


void SynthInput::dumpVoices()
{
  std::cout << "voices:\n";
  for (int i=0; i<_polyphony; ++i)
  {
    std::cout << "    " << i << ": ";
    
    Voice& voice = _voices[i];
    std::cout << "[i: " << voice.creatorID << "]";
    
    switch(voice.state)
    {
      case Voice::kOff:
        std::cout  << "off";
        break;
      case Voice::kOn:
        std::cout  << " on";
        break;
      case Voice::kSustain:
        std::cout  << "sus";
        break;
      default:
        std::cout  << " ? ";
        break;
    }
    std::cout  << "\n";
    
  }
}

void SynthInput::dumpSignals()
{
  std::cout  << "signals:\n";
  for (int i=0; i<_polyphony; ++i)
  {
    std::cout  << "    " << i << ": ";
    
    /*
     // changes per voice
     MLSignal& pitch = getOutput(i*kNumVoiceSignals + 1);
     MLSignal& gate = getOutput(i*kNumVoiceSignals + 2);
     MLSignal& vel = getOutput(i*kNumVoiceSignals + 3);
     MLSignal& voice = getOutput(i*kNumVoiceSignals + 4);
     MLSignal& after = getOutput(i*kNumVoiceSignals + 5);
     
     std::cout << "[pitch: " << pitch[0] << "] ";
     std::cout << "[gate : " << gate[0] << "] ";
     //std::cout << "[vel  : " << vel[0] << "] ";
     //std::cout << "[voice: " << voice[0] << "] ";
     std::cout << "[after: " << after[0] << "] ";
     //std::cout << "\n";
     std::cout << "\n";
     */
    
  }
  
}

}
