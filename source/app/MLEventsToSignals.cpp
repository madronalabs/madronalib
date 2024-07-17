// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLEventsToSignals.h"

namespace ml {

// EventsToSignals::Voice
//

void EventsToSignals::Voice::setParams(float pitchGlideInSeconds, float drift, float sr)
{
  // separate glide time for note pitch
  pitchGlide.setGlideTimeInSamples(sr*pitchGlideInSeconds);
  
  pitchBendGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  modGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  xGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  yGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  zGlide.setGlideTimeInSamples(sr*kGlideTimeSeconds);
  
  pitchDriftGlide.setGlideTimeInSamples(sr*kDriftTimeSeconds);
  driftAmount = drift;
}

// done when DSP is reset.
void EventsToSignals::Voice::reset(int i)
{
  driftSource.mSeed = i*232;
  
  state = kOff;
  nextFrameToProcess = 0;
  ageInSamples = 0;
  ageStep = 0;
  
  currentVelocity = 0;
  currentPitch = 0;
  currentPitchBend = 0;
  currentMod = 0;
  currentX = 0;
  currentY = 0;
  currentZ = 0;
  
  creatorID = 0;
  
  pitchBendGlide.setValue(0.f);
  modGlide.setValue(0.f);
  xGlide.setValue(0.f);
  yGlide.setValue(0.f);
  zGlide.setValue(0.f);
}

// just reset the time.
void EventsToSignals::Voice::resetTime(int i)
{
  ageInSamples = 0;
}

void EventsToSignals::Voice::beginProcess(float sr)
{
  nextFrameToProcess = 0;
  driftCounter += kFloatsPerDSPVector;
  
  int driftIntervalSamples = (int)(sr*kDriftTimeSeconds);
  if(driftCounter >= nextDriftTimeInSamples)
  {
    float d = driftSource.getFloat();
    float nextTimeMul = 1.0f + fabs(driftSource.getFloat());
    currentDriftValue = d;
    driftCounter = 0;
    nextDriftTimeInSamples = (int)(sr * nextTimeMul * kDriftTimeSeconds);
  }
}

float getAgeInSeconds(uint32_t age, float sr)
{
  double dAge(age);
  double dSr(sr);
  double dSeconds = dAge / dSr;
  return (float)dSeconds;
}

void EventsToSignals::Voice::writeNoteEvent(const Event& e, const Scale& scale, float sampleRate)
{
  auto time = e.time;
  
  switch(e.type)
  {
    case kNoteOn:
    {
      //     std::cout << "write note: " << pitchj
      state = kOn;
      creatorID = e.creatorID;
      ageInSamples = 0;
      ageStep = 1;
      int destTime = e.time;
      destTime = clamp(destTime, (0), (int)kFloatsPerDSPVector);
      
      // write current pitch and velocity up to note start
      for(int t = (int)nextFrameToProcess; t < destTime; ++t)
      {
        outputs.row(kGate)[t] = currentVelocity;
        outputs.row(kPitch)[t] = pitchGlide.nextSample(currentPitch);
        ageInSamples += ageStep;
        outputs.row(kElapsedTime)[t] = getAgeInSeconds(ageInSamples, sampleRate);
      }
      
      // set new values
      currentPitch = scale.noteToLogPitch(e.value1);
      currentVelocity = e.value2; // TODO union
      nextFrameToProcess = destTime;
      
      break;
    }
    case kNoteRetrig:
    {
      state = kOn;
      creatorID = e.creatorID;
      int destTime = e.time;
      destTime = clamp(destTime, (0), (int)kFloatsPerDSPVector);
      
      // if the retrigger falls on frame 0, make room for retrigger
      if(destTime == 0)
      {
        destTime++;
      }
      
      // write current pitch and velocity up to retrigger
      for(int t = (int)nextFrameToProcess; t < destTime - 1; ++t)
      {
        outputs.row(kGate)[t] = currentVelocity;
        outputs.row(kPitch)[t] = pitchGlide.nextSample(currentPitch);
        ageInSamples += ageStep;
        outputs.row(kElapsedTime)[t] = getAgeInSeconds(ageInSamples, sampleRate);
      }
      
      // write retrigger frame
      outputs.row(kGate)[destTime - 1] = 0;
      outputs.row(kPitch)[destTime - 1] = pitchGlide.nextSample(currentPitch);
      
      // set new values
      currentPitch = scale.noteToLogPitch(e.value1);
      currentVelocity = e.value2; // TODO union
      nextFrameToProcess = destTime;
      ageInSamples = 0;

      break;
    }
     
    case kNoteSustain:
      state = kSustain;
      break;
      
    case kNoteOff:
    {
      state = kOff;
      creatorID = 0;
      
      size_t destTime = e.time;
      destTime = clamp(destTime, size_t(0), (size_t)kFloatsPerDSPVector);
      
      // write current values up to change TODO DRY
      for(int t = (int)nextFrameToProcess; t < destTime; ++t)
      {
        outputs.row(kGate)[t] = currentVelocity;
        outputs.row(kPitch)[t] = pitchGlide.nextSample(currentPitch);
        ageInSamples += ageStep;
        outputs.row(kElapsedTime)[t] = getAgeInSeconds(ageInSamples, sampleRate);
      }
      
      // set new values
      currentVelocity = 0.;
      nextFrameToProcess = destTime;
      break;
    }
    default:
      state = kOff;
      break;
  }
}

void EventsToSignals::Voice::endProcess(float pitchBend, float sampleRate)
{
  for(int t = (int)nextFrameToProcess; t < kFloatsPerDSPVector; ++t)
  {
    // write velocity to end of buffer.
    outputs.row(kGate)[t] = currentVelocity;
    
    // write pitch to end of buffer.
    outputs.row(kPitch)[t] = pitchGlide.nextSample(currentPitch);
    
    // keep increasing age
    ageInSamples += ageStep;
    outputs.row(kElapsedTime)[t] = getAgeInSeconds(ageInSamples, sampleRate);
  }
  
  // process glides, accurate to the DSP vector
  auto bendGlide = pitchBendGlide(currentPitchBend);
  auto driftSig = pitchDriftGlide(currentDriftValue);
  outputs.row(kMod) = modGlide(currentMod);
  outputs.row(kX) = xGlide(currentX);
  outputs.row(kY) = yGlide(currentY);
  outputs.row(kZ) = zGlide(currentZ);
  
  // add pitch bend in semitones to pitch output
  outputs.row(kPitch) += bendGlide*pitchBend*(1.f/12);
  
  // add drift to pitch output
  outputs.row(kPitch) += driftSig*driftAmount*kDriftScale;
}

#pragma mark -
//
// EventsToSignals
//

EventsToSignals::EventsToSignals(int sr) : _eventQueue(kMaxEventsPerVector)
{
  _sampleRate = (float)sr;
  
  voices.resize(kMaxVoices);
  
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].setParams(_pitchGlideTimeInSeconds, _pitchDriftAmount, (float)sr);
    voices[i].reset(i);
    voices[i].outputs.row(kVoice) = DSPVector((float)i);
  }
}

EventsToSignals::~EventsToSignals()
{
}

size_t EventsToSignals::setPolyphony(int n)
{
  reset();
  _polyphony = std::min(n, kMaxVoices);
  return _polyphony;
}

size_t EventsToSignals::getPolyphony()
{
  return _polyphony;
}

void EventsToSignals::reset()
{
  _eventQueue.clear();
  
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].reset(i);
  }
  
  _lastFreeVoiceFound = -1;
}

void EventsToSignals::resetTimes()
{
  _eventQueue.clear();
  
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].resetTime(i);
  }
  
  _lastFreeVoiceFound = -1;
}

void EventsToSignals::addEvent(const Event& e)
{
  _eventQueue.push(e);
}

void EventsToSignals::process()
{
  for(auto& v : voices)
  {
    v.beginProcess(_sampleRate);
  }
  while(Event e = _eventQueue.pop())
  {
    processEvent(e);
  }
  for(auto& v : voices)
  {
    v.endProcess(kPitchBendSemitones, _sampleRate);
  }
}

// process one incoming event by making the appropriate changes in state and change lists.
void EventsToSignals::processEvent(const Event &eventParam)
{
  Event event = eventParam;
  
  switch(event.type)
  {
    case kNoteOn:
      processNoteOnEvent(event);
      break;
    case kNoteOff:
      processNoteOffEvent(event);
      break;
    case kController:
      processControllerEvent(event);
      break;
    case kPitchWheel:
      processPitchWheelEvent(event);
      break;
    case kNotePressure:
      processNotePressureEvent(event);
      break;
    case kSustainPedal:
      processSustainEvent(event);
      break;
    case kNull:
    default:
      break;
  }
}

void EventsToSignals::processNoteOnEvent(const Event& e)
{
  auto v = findFreeVoice();

  if(v >= 0)
  {
    voices[v].writeNoteEvent(e, _scale, _sampleRate);
  }
  else
  {
    v = findVoiceToSteal(e);
    
    // steal it with retrigger
    // TODO: this may make some clicks when the previous notes
    // are cut off. add more graceful stealing
    Event f = e;
    f.type = kNoteRetrig;
    voices[v].writeNoteEvent(f, _scale, _sampleRate);
  }
  newestVoice = v;
}

void EventsToSignals::processNoteOffEvent(const Event& e)
{
  // send either off or sustain event to voices matching creator
  EventType newEventType = _sustainPedalActive ? kNoteSustain : kNoteOff;
  
  for(int v = 0; v < _polyphony; ++v)
  {
    Voice& voice = voices[v];
    if((voice.creatorID == e.creatorID) && (voice.state == Voice::kOn))
    {
      Event eventToSend = e;
      eventToSend.type = newEventType;
      voice.writeNoteEvent(eventToSend, _scale, _sampleRate);
    }
  }
}

void EventsToSignals::processNotePressureEvent(const Event& event)
{
  for (int v=0; v<_polyphony; ++v)
  {
    voices[v].currentZ = event.value1;
  }
}

void EventsToSignals::processPitchWheelEvent(const Event& event)
{
  for (int v=0; v<_polyphony; ++v)
  {
    // write pitch bend
    voices[v].currentPitchBend = event.value1;
  }
}

// this handles all controller numbers
void EventsToSignals::processControllerEvent(const Event& event)
{
  float val = event.value1;
  int ctrl = (int)event.value2;
  
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
        Voice& voice = voices[v];
        if(voice.state != Voice::kOff)
        {
          Event eventToSend = event;
          eventToSend.type = kNoteOff;
          voice.writeNoteEvent(eventToSend, _scale, _sampleRate);
        }
      }
    }
  }
  else
  {
    // modulate all voices.
    for (int v=0; v<_polyphony; ++v)
    {
      if(ctrl == 1)
      {
        voices[v].currentMod = val;
      }
      if(ctrl == 73)
      {
        voices[v].currentX = val;
      }
      else if(ctrl == 74)
      {
        voices[v].currentY = val;
      }
    }
  }
}

void EventsToSignals::processSustainEvent(const Event& event)
{
  _sustainPedalActive = (event.value1 > 0.5f) ? 1 : 0;
  if(!_sustainPedalActive)
  {
    // clear any sustaining voices
    for(int i=0; i<_polyphony; ++i)
    {
      Voice& v = voices[i];
      if(v.state == Voice::kSustain)
      {
        Event newEvent;
        newEvent.type = kNoteOff;
        v.writeNoteEvent(newEvent, _scale, _sampleRate);
      }
    }
  }
}

void EventsToSignals::setPitchBendInSemitones(float f)
{
  kPitchBendSemitones = f;
}

void EventsToSignals::setGlideTimeInSeconds(float f)
{
  _pitchGlideTimeInSeconds = f;
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].setParams(_pitchGlideTimeInSeconds, _pitchDriftAmount, _sampleRate);
  }
}

void EventsToSignals::setDriftAmount(float f)
{
  _pitchDriftAmount = f;
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].setParams(_pitchGlideTimeInSeconds, _pitchDriftAmount, _sampleRate);
  }
}

#pragma mark -

// return index of free voice or -1 for none.
// increments mVoiceRotateOffset.
//
int EventsToSignals::findFreeVoice()
{
  int len = _polyphony;
  int r = -1;
  int t = _lastFreeVoiceFound;
  for (int i = 0; i < len; ++i)
  {
    t++;
    if(t >= len) t = 0;
    
    if (voices[t].state == Voice::kOff)
    {
      r = t;
      _lastFreeVoiceFound = t;
      break;
    }
  }

  return r;
}

int EventsToSignals::findVoiceToSteal(Event e)
{
  // just steal the voice with the nearest note.
  return findNearestVoice(e.creatorID);
}

// return the index of the voice with the note nearest to the note n.
// Must always return a valid voice index.
int EventsToSignals::findNearestVoice(int note)
{
  int r = 0;
  int minDist = 128;
  
  for (int v=0; v<_polyphony; ++v)
  {
    int vNote = voices[v].creatorID;
    int noteDist = std::abs(note - vNote);
    if (noteDist < minDist)
    {
      minDist = noteDist;
      r = v;
    }
  }
  return r;
}


void EventsToSignals::dumpVoices()
{
  std::cout << "voices:\n";
  for (int i=0; i<_polyphony; ++i)
  {
    std::cout << "    " << i << ": ";
    
    Voice& voice = voices[i];
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

}
 
