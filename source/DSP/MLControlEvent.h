//
//  MLControlEvent.h
//  madronalib
//
//  Created by Randy Jones on 7/9/14.
//
//

#ifndef __MLControlEvent__
#define __MLControlEvent__

#include <iostream>
#include <vector>
#include <stack>

// MLControlEvents are instructions that tell a DSP Engine what to do.
// They can come from different sources such as a score, live code or a live performance.

class MLControlEvent
{
public:
  enum EventType
  {
    kNull = 0,
    kNoteOff,
    kNoteOn,
    kNoteSustain, // when sustain pedal is held, key releases generate sustain events
    kController,
    kPitchWheel,
    kNotePressure,
    kChannelPressure,
    kProgramChange,
    kSustainPedal
  };
  
  MLControlEvent();
  MLControlEvent(EventType type, int channel, int id, uint64_t time, float value, float value2);
  
  void clear();
  bool isFree() const {return mType == kNull;}
  
  // time in samples since DSP engine restart.
  uint64_t mTime;
  
  EventType mType;
  
  int mChannel;
  int mID; // the MIDI key or touch number that created the event. a note-off can match a note-on by ID.
  float mValue1;
  float mValue2;
};

const MLControlEvent kMLNullControlEvent;

class MLControlEventVector :
public std::vector<MLControlEvent>
{
public:
  MLControlEvent& nextFreeEvent() const;
  int findFreeEvent() const;
  void clearEventsMatchingID(int id);
};

// our own stack object is used here because we want to pass by value and control when allocation happens.
class MLControlEventStack :
public std::vector<MLControlEvent>
{
public:
  MLControlEventStack() : mSize(0) {}
  ~MLControlEventStack() {}
  
  // push fails silently when out of memory: don't use when this may be a problem.
  void push(const MLControlEvent&);
  MLControlEvent pop();
  bool isEmpty();
  int getSize();
  void clearEventsMatchingID(int id);
  int mSize;
};

#endif /* defined(__MLControlEvent__) */

