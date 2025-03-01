//
// Created by Randy Jones on 2/25/25.
//

#pragma once


namespace ml {

enum EventType
{
  kNull = 0,
  kNoteOn,
  kNoteRetrig,
  kNoteSustain,
  kNoteOff,
  kSustainPedal, // when sustain pedal is held, key releases generate kNoteSustain events
  kController,
  kPitchBend,
  kNotePressure,
  kChannelPressure,
  kProgramChange
};

// Event: something that happens in a performance.
//
struct Event
{
  Event() = default;
  ~Event() = default;

  EventType type{kNull};
  int channel;
  int keyNumber;  // The unique key or touch number that created the event.
  int time; // Onset time in samples from start of current process buffer.

  // float values that have different meanings for different event types.
  float value1{0};
  float value2{0};
  float value3{0};
  float value4{0};

  explicit operator bool() const { return type != kNull; }
};

} // ml
