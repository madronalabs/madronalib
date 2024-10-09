// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLEventsToSignals.h"

namespace ml {

// EventsToSignals::Voice
//

void EventsToSignals::Voice::setParams(float pitchGlideInSeconds, float drift, float sr)
{
  // store separate glide time for note pitch
  pitchGlideTimeInSamples = sr*pitchGlideInSeconds;
  
  if(!inhibitPitchGlide)
  {
    pitchGlide.setGlideTimeInSamples(pitchGlideTimeInSamples);
  }
  
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
  
  creatorKeyNumber = 0;
  
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

void EventsToSignals::Voice::writeNoteEvent(const Event& e, const Scale& scale, float sampleRate, bool doGlide)
{
  auto time = e.time;
  
  switch(e.type)
  {
    case kNoteOn:
    {
      creatorKeyNumber = e.keyNumber;
      ageInSamples = 0;
      ageStep = 1;
      int destTime = e.time;
      destTime = clamp(destTime, (0), (int)kFloatsPerDSPVector);
      
      inhibitPitchGlide = !doGlide;
      if(!inhibitPitchGlide)
      {
        pitchGlide.setGlideTimeInSamples(pitchGlideTimeInSamples);
      }
      else
      {
        pitchGlide.setGlideTimeInSamples(0);
      }
      
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
      currentVelocity = e.value2;
      nextFrameToProcess = destTime;
      
      break;
    }
    case kNoteRetrig:
    {
      creatorKeyNumber = e.keyNumber;
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
     

    case kNoteOff:
    {
      creatorKeyNumber = 0;
      
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

EventsToSignals::EventsToSignals(int sr) : eventQueue_(kMaxEventsPerVector)
{
  sampleRate_ = (float)sr;
  
  voices.resize(kMaxVoices);
  
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].setParams(pitchGlideTimeInSeconds_, pitchDriftAmount_, (float)sr);
    voices[i].reset(i);
    voices[i].outputs.row(kVoice) = DSPVector((float)i);
  }
}

EventsToSignals::~EventsToSignals()
{
}

size_t EventsToSignals::setPolyphony(size_t n)
{
  reset();
  polyphony_ = std::min(n, kMaxVoices);
  return polyphony_;
}

size_t EventsToSignals::getPolyphony()
{
  return polyphony_;
}

void EventsToSignals::reset()
{
  eventQueue_.clear();
  
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].reset(i);
  }
  
  lastFreeVoiceFound_ = -1;
}

void EventsToSignals::resetTimes()
{
  eventQueue_.clear();
  
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].resetTime(i);
  }
  
  lastFreeVoiceFound_ = -1;
}

void EventsToSignals::addEvent(const Event& e)
{
  eventQueue_.push(e);
}

void EventsToSignals::process()
{
  for(auto& v : voices)
  {
    v.beginProcess(sampleRate_);
  }
  while(Event e = eventQueue_.pop())
  {
    processEvent(e);
  }
  for(auto& v : voices)
  {
    v.endProcess(pitchBendRangeInSemitones_, sampleRate_);
  }
  
  // TEMP
  testCounter += kFloatsPerDSPVector;
  if(testCounter > sampleRate_)
  {
 //   dumpVoices();
    testCounter -= sampleRate_;
  }
}

size_t EventsToSignals::countHeldNotes()
{
  // count held notes. It might seem like we could just keep a counter, but
  // redundant note offs, which break that approach, are common.
  size_t heldNotes{0};
  for(int i=0; i<kMaxPhysicalKeys; ++i)
  {
    const auto ks = keyStates_[i];
    if(ks.state == KeyState::kOn) heldNotes++;
  }
  return heldNotes;
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
  keyStates_[e.keyNumber].state = KeyState::kOn;
  keyStates_[e.keyNumber].noteOnIndex = currentNoteOnIndex++;
  keyStates_[e.keyNumber].pitch = e.value1;
    
  if(unison_)
  {
    // don't glide to first note played in unison mode.
    bool glideToNextNote = (countHeldNotes() > 1);
    
    for(int v=0; v<polyphony_; ++v)
    {
      voices[v].writeNoteEvent(e, scale_, sampleRate_, glideToNextNote);
    }
  }
  else
  {
    auto v = findFreeVoice();
    
    if(v >= 0)
    {
      voices[v].writeNoteEvent(e, scale_, sampleRate_);
    }
    else
    {
      v = findVoiceToSteal(e);
      
      // steal it with retrigger
      // TODO: this may make some clicks when the previous notes
      // are cut off. add more graceful stealing
      Event f = e;
      f.type = kNoteRetrig;
      voices[v].writeNoteEvent(f, scale_, sampleRate_);
    }
    newestVoice_ = v;
  }
}

void EventsToSignals::processNoteOffEvent(const Event& e)
{
  if(sustainPedalActive_)
  {
    keyStates_[e.keyNumber].state = KeyState::kSustain;
  }
  else
  {
    keyStates_[e.keyNumber].state = KeyState::kOff;
  }

  if(unison_)
  {
    // if the last note was released, turn off all voices.
    if(countHeldNotes() == 0)
    {
      for(int v=0; v<polyphony_; ++v)
      {
        voices[v].writeNoteEvent(e, scale_, sampleRate_);
      }
    }
    else
    {
      // if the note released is the currently playing one, change all voices
      // to the most recently played held note.
      if (e.keyNumber == voices[0].creatorKeyNumber)
      {
        // send kNoteOn event to change note without retriggering envelope
        Event eventToSend = e;
        eventToSend.type = kNoteOn;
        
        // keep current velocity for all voices
        eventToSend.value2 = voices[0].currentVelocity;
        
        // get most recently played held key
        uint32_t maxNoteOnIndex{0};
        uint32_t mostRecentHeldKey{0};
        for(int i=0; i<kMaxPhysicalKeys; ++i)
        {
          const auto ks = keyStates_[i];
          if((ks.state == KeyState::kOn) && (ks.noteOnIndex > maxNoteOnIndex))
          {
            maxNoteOnIndex = ks.noteOnIndex;
            mostRecentHeldKey = i;
          }
        }
        
        // send key number and pitch of most recent held note
        eventToSend.keyNumber = mostRecentHeldKey;
        eventToSend.value1 = keyStates_[mostRecentHeldKey].pitch;
       
        for(int v=0; v<polyphony_; ++v)
        {
          voices[v].writeNoteEvent(eventToSend, scale_, sampleRate_);
        }
      }
    }
  }
  else
  {
    if(!sustainPedalActive_)
    {
      for(int v = 0; v < polyphony_; ++v)
      {
        if(voices[v].creatorKeyNumber == e.keyNumber)
        {
          voices[v].writeNoteEvent(e, scale_, sampleRate_);
        }
      }
    }
  }
}


void EventsToSignals::processNotePressureEvent(const Event& event)
{
  for (int v=0; v<polyphony_; ++v)
  {
    voices[v].currentZ = event.value1;
  }
}

void EventsToSignals::processPitchWheelEvent(const Event& event)
{
  for (int v=0; v<polyphony_; ++v)
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
      for(int v=0; v<polyphony_; ++v)
      {
        Voice& voice = voices[v];
        Event eventToSend = event;
        eventToSend.type = kNoteOff;
        voice.writeNoteEvent(eventToSend, scale_, sampleRate_);
      }
    }
  }
  else
  {
    // modulate all voices.
    for (int v=0; v<polyphony_; ++v)
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
  sustainPedalActive_ = (event.value1 > 0.5f) ? 1 : 0;
  if(!sustainPedalActive_)
  {
    // clear any sustaining voices
    for(int i=0; i<polyphony_; ++i)
    {
      Voice& v = voices[i];
      if(keyStates_[v.creatorKeyNumber].state == KeyState::kSustain)
      {
        Event newEvent;
        newEvent.type = kNoteOff;
        v.writeNoteEvent(newEvent, scale_, sampleRate_);
      }
    }
  }
}

void EventsToSignals::setPitchBendInSemitones(float f)
{
  pitchBendRangeInSemitones_ = f;
}

void EventsToSignals::setGlideTimeInSeconds(float f)
{
  pitchGlideTimeInSeconds_ = f;
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].setParams(pitchGlideTimeInSeconds_, pitchDriftAmount_, sampleRate_);
  }
}

void EventsToSignals::setDriftAmount(float f)
{
  pitchDriftAmount_ = f;
  for(int i=0; i<kMaxVoices; ++i)
  {
    voices[i].setParams(pitchGlideTimeInSeconds_, pitchDriftAmount_, sampleRate_);
  }
}

void EventsToSignals::setUnison(bool b)
{
  unison_ = b;
}

#pragma mark -

// return index of free voice or -1 for none.
// if a free voice is found, increments lastFreeVoiceFound_.
//
int EventsToSignals::findFreeVoice()
{
  int len = polyphony_;
  int r = -1;
  int t = lastFreeVoiceFound_;
  for (int i = 0; i < len; ++i)
  {
    t++;
    if(t >= len) t = 0;
    
    if (voices[t].creatorKeyNumber == 0)
    {
      r = t;
      lastFreeVoiceFound_ = t;
      break;
    }
  }
  
  return r;
}

int EventsToSignals::findVoiceToSteal(Event e)
{
  // just steal the voice with the nearest note.
  return findNearestVoice(e.keyNumber);
}

// return the index of the voice with the note nearest to the note n.
// Must always return a valid voice index.
int EventsToSignals::findNearestVoice(int note)
{
  int r = 0;
  int minDist = 128;
  
  for (int v=0; v<polyphony_; ++v)
  {
    int vNote = voices[v].creatorKeyNumber;
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
  for (int i=0; i<polyphony_; ++i)
  {
    std::cout << "    " << i << ": ";
    
    Voice& voice = voices[i];
    int vKey = voice.creatorKeyNumber;
    std::cout << "[i: " << vKey << "]";
    
    switch(keyStates_[vKey].state)
    {
      case KeyState::kOff:
        std::cout  << "off";
        break;
      case KeyState::kOn:
        std::cout  << " on";
        break;
      case KeyState::kSustain:
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
 
