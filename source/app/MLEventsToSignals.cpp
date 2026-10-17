// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSymbol.h"
#include "MLEventsToSignals.h"
#include <cassert>


namespace ml
{

float samplesToSeconds(uint32_t samples, float sr)
{
  double ns(samples);
  double samplesPerSecond(sr);
  double seconds = ns / samplesPerSecond;
  return (float)seconds;
}

int getKeyIndex(const Event& e, Symbol protocol)
{
  int instigator{0};
  switch(hash(protocol))
  {
    case(hash("MIDI")):
    {
      instigator = e.sourceIdx;
      break;
    }
    case(hash("MPE")):
    {
      instigator = e.channel;
      break;
    }
    default:
    {
      break;
    }
  }

  // keyStates_ is indexed with this result directly, so it can never be allowed
  // out of range. Note that a caller passing a negative key does not arrive
  // here as a negative number: sourceIdx is uint16_t and channel is uint8_t, so
  // CLAP's -1 wildcard shows up as 65535 or 255. Clamping is the only backstop.
  return ml::clamp(instigator, 0, (int)EventsToSignals::kMaxPhysicalKeys - 1);
}

#pragma mark -
// EventsToSignals::Voice
//

void EventsToSignals::Voice::setSampleRate(double r)
{
  sr = r;
  recalcNeeded = true;
}

void EventsToSignals::Voice::setPitchGlideInSeconds(float g)
{
  pitchGlideTimeInSeconds = g;
  recalcNeeded = true;
}

void EventsToSignals::Voice::setDriftAmount(float d) { driftAmount = d; }

// done when DSP is reset: a polyphony or protocol change, a host reset, or
// all sound off. The note ends and bend, mod and touch go to zero, but the voice
// keeps its pitch, as a hardware pitch CV stays where it was. A voice that has
// never had a note stays at 0.
void EventsToSignals::Voice::reset()
{
  driftSource.seed_ = voiceIndex * 232;

  nextFrameToProcess = 0;
  eventAgeInSamples = 0;
  eventAgeStep = 0;

  currentVelocity = 0;
  currentPitchBend = 0;
  currentMod = 0;
  currentX = 0;
  currentY = 0;
  currentZ = 0;

  creatorKeyIdx_ = 0;
  creatorChannel_ = 0;

  pitchBendGlide.setValue(0.f);
  modGlide.setValue(0.f);
  xGlide.setValue(0.f);
  yGlide.setValue(0.f);
  zGlide.setValue(0.f);
}

// just reset the time.
void EventsToSignals::Voice::resetTime() { eventAgeInSamples = 0; }

void EventsToSignals::Voice::beginProcess()
{
  if (recalcNeeded)
  {
    isr = 1.0 / sr;

    // store separate glide time for note pitch
    pitchGlideTimeInSamples = sr * pitchGlideTimeInSeconds;

    if (!inhibitPitchGlide)
    {
      pitchGlide.setGlideTimeInSamples(pitchGlideTimeInSamples);
    }

    pitchBendGlide.setGlideTimeInSamples(sr * kGlideTimeSeconds);
    modGlide.setGlideTimeInSamples(sr * kGlideTimeSeconds);
    xGlide.setGlideTimeInSamples(sr * kGlideTimeSeconds);
    yGlide.setGlideTimeInSamples(sr * kGlideTimeSeconds);
    zGlide.setGlideTimeInSamples(sr * kGlideTimeSeconds);

    pitchDriftGlide.setGlideTimeInSamples(sr * kDriftTimeSeconds);

    recalcNeeded = false;
  }

  nextFrameToProcess = 0;
  driftCounter += kFramesPerBlock;

  int driftIntervalSamples = (int)(sr * kDriftTimeSeconds);
  if (driftCounter >= nextDriftTimeInSamples)
  {
    float d = driftSource.getFloat();
    float nextTimeMul = 1.0f + fabs(driftSource.getFloat());
    currentDriftValue = d;
    driftCounter = 0;
    nextDriftTimeInSamples = (int)(sr * nextTimeMul * kDriftTimeSeconds);
  }
}

// write current signals and continue existing glides up to the frame just before
// the time when a new event will happen.
void EventsToSignals::Voice::writeOutputSignals(size_t endTime)
{
  while(nextFrameToProcess < endTime)
  {
    // write velocity to end of buffer.
    outputs.row(kGate)[nextFrameToProcess] = currentVelocity;
    
    // write pitch to end of buffer.
    outputs.row(kPitch)[nextFrameToProcess] = pitchGlide.nextSample(currentPitch);
    
    // keep increasing age
    eventAgeInSamples += eventAgeStep;
    outputs.row(kElapsedTime)[nextFrameToProcess] = samplesToSeconds(eventAgeInSamples, sr);
    
    nextFrameToProcess++;
  }
}

// Write a note event to the voice. Writes all voice output signals for all frames
// prior to the event time. Updates nextFrameToProcess with the event time.
void EventsToSignals::Voice::writeNoteEvent(const Event& e, int keyIdx, bool doGlide, bool doReset)
{
  // incoming time in the event e is the sample offset into the SignalBlock.
  size_t destTime = clamp((size_t)e.time, (size_t)0, (size_t)kFramesPerBlock);

  switch (e.type)
  {
    case kNoteOn:
    {
      if (doReset)
      {
        eventAgeInSamples = 0;
      }
      eventAgeStep = 1;

      inhibitPitchGlide = !doGlide;
      if (doGlide)
      {
        pitchGlide.setGlideTimeInSamples(pitchGlideTimeInSamples);
      }
      else
      {
        pitchGlide.setGlideTimeInSamples(0);
      }

      writeOutputSignals(destTime);

      // set new values
      creatorKeyIdx_ = keyIdx;
      creatorChannel_ = e.channel;
      currentPitch = e.value1;
      currentVelocity = e.value2;
      break;
    }
    case kNoteRetrig:
    {
      if (doReset)
      {
        eventAgeInSamples = 0;
      }
      eventAgeStep = 1;

      // if the retrigger falls on frame 0, make room for retrigger
      if (destTime == 0)
      {
        destTime++;
      }

      // write previous values up to retrigger frame
      writeOutputSignals(destTime - 1);

      // write retrigger frame
      currentVelocity = 0;
      writeOutputSignals(destTime);

      // set new values
      creatorKeyIdx_ = keyIdx;
      creatorChannel_ = e.channel;
      currentPitch = e.value1;
      currentVelocity = e.value2;
      break;
    }
    case kNoteSustain:
    {
      writeOutputSignals(destTime);
      break;
    }
    case kNoteOff:
    {
      creatorKeyIdx_ = 0;
      writeOutputSignals(destTime);

      // set new values
      currentVelocity = 0.;
      break;
    }
    default:
      break;
  }
}

// write all voice output signals from the most recent event's time to
// the end of the buffer.
void EventsToSignals::Voice::endProcess(float pitchBend)
{
  writeOutputSignals(kFramesPerBlock);

  // process glides, accurate to the DSP vector
  auto bendGlide = pitchBendGlide(currentPitchBend);
  auto driftSig = pitchDriftGlide(currentDriftValue);
  outputs.setRow(kMod, modGlide(currentMod));
  outputs.setRow(kX, xGlide(currentX));
  outputs.setRow(kY, yGlide(currentY));

  if (currentVelocity == 0.f)
  {
    currentZ = 0.f;
  }
  outputs.setRow(kZ, zGlide(currentZ));

  // add pitch bend in semitones to pitch output, and write it to its own row
  // so clients can separate it from the note
  auto bend = bendGlide * pitchBend * (1.f / 12);
  outputs.setRow(kBend, bend);
  auto p = outputs.getRow(kPitch);
  outputs.setRow(kPitch, p + bend);

  // add drift to pitch output
  p = outputs.getRow(kPitch);
  outputs.setRow(kPitch, p + driftSig * driftAmount * kDriftScale);
}

#pragma mark -
//
// SmoothedController
//

void EventsToSignals::SmoothedController::setSampleRate(double r)
{
  sr = r;
  recalcNeeded = true;
}

void EventsToSignals::SmoothedController::process()
{
  if (recalcNeeded)
  {
    // send sample rate to controller glides.
    int glideTimeInSamples = sr * kControllerGlideTimeSeconds;
    glide.setGlideTimeInSamples(glideTimeInSamples);
    recalcNeeded = false;
  }

  output = glide(inputValue);
}

#pragma mark -
//
// EventsToSignals
//

EventsToSignals::EventsToSignals()
{
  eventBuffer_.reserve(kMaxEventsPerProcessBuffer);

  voices.resize(kMaxVoices + 1);
  for (int i = 0; i < voices.size(); ++i)
  {
    voices[i].voiceIndex = i;
    voices[i].reset();

    // set vox output signal
    voices[i].outputs.setRow(kVoice, SignalBlock((float)i - 1));
  }

  controllers.resize(kNumControllers);
}

EventsToSignals::~EventsToSignals() {}

void EventsToSignals::setSampleRate(double r)
{
  sr = r;

  for (auto& v : voices)
  {
    v.setSampleRate(r);
  }

  for (auto& c : controllers)
  {
    c.setSampleRate(r);
  }
}

size_t EventsToSignals::setPolyphony(size_t n)
{
  clear();
  polyphony_ = std::min(n, kMaxVoices);
  return polyphony_;
}

size_t EventsToSignals::getPolyphony() { return polyphony_; }

void EventsToSignals::clear()
{
  eventBuffer_.clear();

  for (auto& v : voices)
  {
    v.reset();
  }

  lastFreeVoiceFound_ = 0;
}

void EventsToSignals::resetTimes()
{
  eventBuffer_.clear();

  for (auto& v : voices)
  {
    v.resetTime();
  }

  lastFreeVoiceFound_ = 0;
}

// sort by time for buffer insertion.
bool soonerThan(const Event& a, const Event& b)
{
  if (a.time != b.time)
  {
    return a.time < b.time;
  }
  else
  {
    // types should be in enum in temporal order. The only crucial
    // order is that a kOff must come after a kOn at the same time.
    return a.type < b.type;
  }
}

// events should usually arrive in order, but unfortunately not all hosts will ensure this.
// so we need to insert events by time on arrival.
void EventsToSignals::addEvent(const Event& e)
{
  awake_ = true;
  auto it = std::lower_bound(eventBuffer_.begin(), eventBuffer_.end(), e, soonerThan);
  eventBuffer_.insert(it, e);
}

void EventsToSignals::clearEvents() {
  eventBuffer_.clear();
}

void EventsToSignals::adjustEventsInBuffer(size_t startTime)
{
  for (auto& e : eventBuffer_)
  {
    e.time -= startTime;
  }
}


void EventsToSignals::makeSignalBlock()
{
  // if we have never received an event, do nothing
  if (!awake_) return;

  // if we have never received a sample rate, do nothing
  if (sr == 0.f) return;

  // start processing each voice's vector of audio data
  for (auto& v : voices)
  {
    v.beginProcess();
  }

  // pop any events in the buffer that are within the first signal block,
  // sending changes to voices and controller smoothers
  int eventsProcessed = 0;
  for (const auto& e : eventBuffer_)
  {
    if(within(e.time, 0, (int)kFramesPerBlock))
    {
      Event eventInThisVector = e;
      processEvent(eventInThisVector);
      eventsProcessed++;
    }
  }
  eventBuffer_.erase(eventBuffer_.begin(), eventBuffer_.begin() + eventsProcessed);

  // end voice processing, making complete outgoing signals
  // MPE main voice (index 0) uses MIDI pitch bend setting
  //
  // TODO: this is a bit ugly, set pitch bend elsewhere
  voices[0].endProcess(pitchBendRangeInSemitones_);
  float voicesPitchBend =
      (protocol_ == "MPE") ? mpePitchBendRangeInSemitones_ : pitchBendRangeInSemitones_;
  for (int v = 1; v < polyphony_ + 1; ++v)
  {
    voices[v].endProcess(voicesPitchBend);
  }

  // make smoothed controller signals
  for (auto& c : controllers)
  {
    c.process();
  }

  // in MIDI mode, add smoothed Channel Pressure to z output
  // in MPE mode, add main voice signals to other voices
  switch(hash(protocol_))
  {
    case(hash("MIDI")):
    {
      for (int v = 1; v < polyphony_ + 1; ++v)
      {
        voices[v].outputs.row(kZ) += controllers[kChannelPressureControllerIdx].output;
      }
      break;
    }
    case(hash("MPE")):
    {
      for (int v = 1; v < polyphony_ + 1; ++v)
      {
        voices[v].outputs.row(kPitch) += voices[0].outputs.row(kPitch);
        voices[v].outputs.row(kBend) += voices[0].outputs.row(kBend);
        voices[v].outputs.row(kX) += voices[0].outputs.row(kX);
        voices[v].outputs.row(kY) += voices[0].outputs.row(kY);
        voices[v].outputs.row(kZ) += voices[0].outputs.row(kZ);
        voices[v].outputs.row(kMod) += voices[0].outputs.row(kMod);
      }
      break;
    }
    default:
    {
      break;
    }
  }

  testCounter += kFramesPerBlock;
  const int samples = 48000*5;

  if (testCounter > samples)
  {
    //dumpVoices();
    testCounter -= samples;
  }
}

size_t EventsToSignals::countHeldKeys()
{
  // count held keys. It might seem like we could just keep a counter, but
  // redundant note offs, which break that approach, are common.
  size_t heldNotes{0};
  for (int i = 0; i < kMaxPhysicalKeys; ++i)
  {
    const auto ks = keyStates_[i];
    if (ks.state == KeyState::kOn) heldNotes++;
  }
  return heldNotes;
}

// TEMP
void EventsToSignals::showHeldKeys()
{
  std::cout << "held:\n";
  for (int i = 0; i < kMaxPhysicalKeys; ++i)
  {
    if(((i % 4) == 0) && i)
    {
      std::cout << " ";
    }
    const auto ks = keyStates_[i].state;
    if (ks == KeyState::kOn)
    {
      std::cout << "*";
    }
    else if (ks == KeyState::kSustained)
    {
      std::cout << "S";
    }
    else
    {
      std::cout << "-";
    }
  }
  std::cout << "\n";
}

size_t EventsToSignals::countActiveVoices()
{
  size_t c{0};
  for (int i = 1; i < polyphony_ + 1; ++i)
  {
    if (voices[i].creatorKeyIdx_ > 0)
    {
      c++;
    }
  }
  return c;
}

// process one incoming event by making the appropriate changes in state and change lists.
void EventsToSignals::processEvent(const Event& eventParam)
{
  Event event = eventParam;

  switch (event.type)
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
    case kPitchBend:
      processPitchWheelEvent(event);
      break;
    case kNotePressure:
      processNotePressureEvent(event);
      break;
    case kChannelPressure:
      processChannelPressureEvent(event);
      break;
    case kSustainPedal:
      processSustainPedalEvent(event);
      break;
    case kNull:
    default:
      break;
  }
}

// a note on event tells use that the given key (channel, key#) wants to start
// a note with the given pitch and velocity.
void EventsToSignals::processNoteOnEvent(const Event& e)
{
  int keyIdx = getKeyIndex(e, protocol_);
  keyStates_[keyIdx].state = KeyState::kOn;
  keyStates_[keyIdx].noteOnIndex = currentNoteOnIndex++;
  keyStates_[keyIdx].pitch = e.value1;

  
  // std::cout << "processNoteOnEvent: " << keyIdx << "\n";

  
  if (unison_)
  {
    // don't glide to first note played in unison mode.
    bool firstNote = (countHeldKeys() == 1);

    // start after MPE main voice
    for (int v = 1; v < polyphony_ + 1; ++v)
    {
      voices[v].writeNoteEvent(e, keyIdx, !firstNote, firstNote);
    }
  }
  else
  {
    Event eventToWrite = e;
    auto v = findFreeVoice();
    
    // if no free voice was found, steal the nearest voice.
    if (v < 1)
    {
      v = findNearestVoice(e.sourceIdx);

      // steal it with retrigger
      // TODO: this may make some clicks when the previous notes
      // are cut off. add more graceful stealing
      eventToWrite.type = kNoteRetrig;
    }
    
    voices[v].writeNoteEvent(eventToWrite, keyIdx, true, true);
    newestVoice_ = v;
    
    
    // TEMP
    /*
    auto heldKeys = countHeldKeys();
    auto voicesOn = countActiveVoices();
    std::cout << "ON: voice:" << v << " key:" << keyIdx <<  " time:" << eventToWrite.time
    << " held:" << heldKeys << " active:" << voicesOn << "\n";
    showHeldKeys();
     */
  }
}

void EventsToSignals::processNoteOffEvent(const Event& e)
{
  int keyIdx = getKeyIndex(e, protocol_);
  if (sustainPedalActive_)
  {
    keyStates_[keyIdx].state = KeyState::kSustained;
  }
  else
  {
    keyStates_[keyIdx].state = KeyState::kOff;
  }
  
  
 // std::cout << "processNoteOffEvent: " << keyIdx << "\n";

  if (unison_)
  {
    // if the last note was released, turn off all voices.
    if (countHeldKeys() == 0)
    {
      for (int v = 1; v < polyphony_ + 1; ++v)
      {
        voices[v].writeNoteEvent(e, 0, true, true);
      }
    }
    else
    {
      // if the note released is the currently playing one, change all voices
      // to the most recently played held note.
      if (keyIdx == voices[1].creatorKeyIdx_)
      {
        // send kNoteOn event to change note without retriggering envelope
        Event eventToSend = e;
        eventToSend.type = kNoteOn;

        // keep current velocity for all voices
        eventToSend.value2 = voices[1].currentVelocity;

        // get most recently played held key
        uint32_t maxNoteOnIndex{0};
        uint32_t mostRecentHeldKey{0};
        for (int i = 0; i < kMaxPhysicalKeys; ++i)
        {
          const auto& ks = keyStates_[i];
          if ((ks.state == KeyState::kOn) && (ks.noteOnIndex > maxNoteOnIndex))
          {
            maxNoteOnIndex = ks.noteOnIndex;
            mostRecentHeldKey = i;
          }
        }

        // send key number and pitch of most recent held note
        eventToSend.value1 = keyStates_[mostRecentHeldKey].pitch;
        for (int v = 1; v < polyphony_ + 1; ++v)
        {
          voices[v].writeNoteEvent(eventToSend, mostRecentHeldKey, true, true);
        }
      }
    }
  }
  else
  {
    Event eventToWrite = e;
    eventToWrite.type = sustainPedalActive_ ? kNoteSustain : kNoteOff;
    for (int v = 1; v < polyphony_ + 1; ++v)
    {
      if (voices[v].creatorKeyIdx_ == keyIdx)
      {
        voices[v].writeNoteEvent(eventToWrite, keyIdx, true, true);

        // TEMP
        /*
        auto heldKeys = countHeldKeys();
        auto voicesOn = countActiveVoices();
        std::cout << "OFF: voice:" << v << " key:" << keyIdx <<  " time:" << e.time
        << " held:" << heldKeys << " active:" << voicesOn << "\n";
         */
      }
    }
  }
}

// ?
void EventsToSignals::processNoteUpdateEvent(const Event& event) {}

void EventsToSignals::processChannelPressureEvent(const Event& event)
{
  switch(hash(protocol_))
  {
    case(hash("MIDI")):
    {
      float val = event.value1;
      controllers[kChannelPressureControllerIdx].inputValue = val;
      break;
    }
    case(hash("MPE")):
    {
      if (event.channel == 1)
      {
        // write main MPE channel
        voices[0].currentZ = event.value1;
      }
      else if (event.channel != 0)
      {
        // write any voice matching channel
        for (int v = 1; v < polyphony_ + 1; ++v)
        {
          if (voices[v].creatorKeyIdx_ == event.channel)
          {
            voices[v].currentZ = event.value1;
          }
        }
      }

      break;
    }
  }
}

void EventsToSignals::processNotePressureEvent(const Event& event)
{
  switch(hash(protocol_))
  {
    case(hash("MIDI")):
    {
      // write any voice matching key
      for (int v = 1; v < polyphony_ + 1; ++v)
      {
        if (voices[v].creatorKeyIdx_ == event.sourceIdx)
        {
          voices[v].currentZ = event.value1;
        }
      }
      break;
    }
    case(hash("MPE")):
    {
      // note pressure is ignored in MPE mode as per the spec.
      break;
    }
  }
}

void EventsToSignals::processPitchWheelEvent(const Event& event)
{
  switch(hash(protocol_))
  {
    case(hash("MIDI")):
    {
      for (int v = 1; v < polyphony_ + 1; ++v)
      {
        voices[v].currentPitchBend = event.value1;
      }
      break;
    }
    case(hash("MPE")):
    {
      if (event.channel == 1)
      {
        // write main MPE voice
        voices[0].currentPitchBend = event.value1;
      }
      else if (event.channel != 0)
      {
        // write any voice matching channel
        for (int v = 1; v < polyphony_ + 1; ++v)
        {
          if (voices[v].creatorKeyIdx_ == event.channel)
          {
            voices[v].currentPitchBend = event.value1;
          }
        }
      }
      break;
    }
  }
}

// this handles all controller numbers
void EventsToSignals::processControllerEvent(const Event& event)
{
  int chan = event.channel;
  float val = event.value1;

  // store values directly into array so they can be read by clients
  size_t ctrl = clamp(size_t(event.sourceIdx), (size_t)0, kNumControllers - 1);
  controllers[ctrl].inputValue = val;

  // handle special meanings for some MIDI controllers
  if (ctrl == 120)
  {
    if (val == 0)
    {
      // all sound off. this runs inside makeSignalBlock()'s loop over eventBuffer_, so reset
      // the voices without clear(), which would empty the buffer out from under that loop.
      for (auto& v : voices)
      {
        v.reset();
      }
      lastFreeVoiceFound_ = 0;
    }
  }
  else if (ctrl == 123)
  {
    if (val == 0)
    {
      // all notes off
      for (auto& v : voices)
      {
        Event eventToSend = event;
        eventToSend.type = kNoteOff;
        v.writeNoteEvent(eventToSend, 0, false, true);
      }
    }
  }
  else
  {
    switch(hash(protocol_))
    {
      case(hash("MIDI")):
      {
        // modulate all voices.
        for (int v = 1; v < polyphony_ + 1; ++v)
        {
          if (ctrl == voiceModCC_)
          {
            voices[v].currentMod = val;
          }
          if (ctrl == 73)
          {
            voices[v].currentX = val;
          }
          else if (ctrl == 74)
          {
            voices[v].currentY = val;
          }
        }
        break;
      }
      case(hash("MPE")):
      {
        // modulate main voice
        if (event.channel == 1)
        {
          if (ctrl == voiceModCC_)
          {
            // modulate main MPE voice
            voices[0].currentMod = val;
          }
        }
        else
        {
          // modulate other voices matching event channel.
          for (int v = 1; v < polyphony_ + 1; ++v)
          {
            if (voices[v].creatorKeyIdx_ == event.channel)
            {
              if (ctrl == voiceModCC_)
              {
                voices[v].currentMod = val;
              }
              if (ctrl == 73)
              {
                voices[v].currentX = val;
              }
              else if (ctrl == 74)
              {
                voices[v].currentY = val;
              }
            }
          }
        }
        break;
      }
    }
  }
}

void EventsToSignals::processSustainPedalEvent(const Event& event)
{
  sustainPedalActive_ = (event.value1 > 0.5f) ? 1 : 0;
  if (!sustainPedalActive_)
  {
    // clear sustained voices
    for (int i = 1; i < polyphony_ + 1; ++i)
    {
      Voice& v = voices[i];
      if (keyStates_[v.creatorKeyIdx_].state == KeyState::kSustained)
      {
        Event newEvent;
        newEvent.type = kNoteOff;
        v.writeNoteEvent(newEvent, 0, true, true);
      }
    }
    
    // clear sustained key states
    for (int i = 0; i < kMaxPhysicalKeys; ++i)
    {
      auto& ks = keyStates_[i];
      if (ks.state == KeyState::kSustained)
      {
        ks.state = KeyState::kOff;
      }
    }
  }
}

void EventsToSignals::setPitchBendInSemitones(float f) { pitchBendRangeInSemitones_ = f; }

void EventsToSignals::setMPEPitchBendInSemitones(float f) { mpePitchBendRangeInSemitones_ = f; }

void EventsToSignals::setPitchGlideInSeconds(float f)
{
  for (auto& v : voices)
  {
    v.setPitchGlideInSeconds(f);
  }
}

void EventsToSignals::setDriftAmount(float f)
{
  for (auto& v : voices)
  {
    v.driftAmount = f;
  }
}

void EventsToSignals::setUnison(bool b) { unison_ = b; }

#pragma mark -

// return (one-based) index of free voice or -1 for none.
// if a free voice is found, lastFreeVoiceFound_ is updated.
//
int EventsToSignals::findFreeVoice()
{
  auto highestVoiceIdx = polyphony_ + 1;
  int r = -1;
  int t = lastFreeVoiceFound_;
  for (int i = 1; i < polyphony_ + 1; ++i)
  {
    t++;
    if (t >= highestVoiceIdx) t = 1;

    if (voices[t].creatorKeyIdx_ == 0)
    {
      r = t;
      lastFreeVoiceFound_ = t;
      break;
    }
  }

  return r;
}

// return the index of the voice with the note nearest to the note n.
// Must always return a valid voice index.
int EventsToSignals::findNearestVoice(int note)
{
  int r = 1;
  size_t minDist = 1024;

  for (int v = 1; v < polyphony_ + 1; ++v)
  {
    size_t vNote = voices[v].creatorKeyIdx_;
    size_t noteDist = std::abs(note - static_cast<int>(vNote));
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
  auto dumpVoice = [&](int i)
  {
    std::cout << "    " << i << ": ";

    Voice& voice = voices[i];
    size_t vKey = voice.creatorKeyIdx_;
    std::cout << "[key: " << vKey << "] ";

    switch (keyStates_[vKey].state)
    {
      case KeyState::kOff:
        std::cout << "off";
        break;
      case KeyState::kOn:
        std::cout << " on";
        break;
      case KeyState::kSustained:
        std::cout << "sus";
        break;
      default:
        std::cout << " ? ";
        break;
    }

    std::cout << " x:" << voice.outputs.rowPtr(kX)[0];
    std::cout << " y:" << voice.outputs.rowPtr(kY)[0];
    std::cout << " z:" << voice.outputs.rowPtr(kZ)[0];
    std::cout << " pitch:" << voice.outputs.rowPtr(kPitch)[0];
    std::cout << " mod:" << voice.outputs.rowPtr(kMod)[0];

    std::cout << "\n";
  };

  std::cout << "\n\npolyphony: " << polyphony_ << "\n";

  if (protocol_ == "MPE")
  {
    std::cout << "MPE main voice:\n";

    dumpVoice(0);
  }

  std::cout << "channel voices:\n";
  for (int i = 1; i < polyphony_ + 1; ++i)
  {
    dumpVoice(i);
  }

  std::cout << "\n";
}

}  // namespace ml
