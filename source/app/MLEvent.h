//
// Created by Randy Jones on 2/25/25.
//

#pragma once

#include <iostream>

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
  kProgramChange,
  kNumEventTypes
};

// Event: something that happens in a performance.
//
struct Event
{
  Event() = default;
  ~Event() = default;

  uint8_t type{kNull};
  uint8_t channel{0};
  uint16_t keyNumber{0};  // The unique key or touch or controller that created the event.
  int time{0}; // Onset time in samples from start of current top-level buffer.

  // float values that have different meanings for different event types.
  float value1{0};
  float value2{0};

  explicit operator bool() const { return type != kNull; }
  
  static const char* typeNames[kNumEventTypes]; 
};


inline std::ostream& operator<<(std::ostream& out, const Event& e)
{
  std::cout << "[" << Event::typeNames[e.type] << " " << (int)e.channel << "/" << (int)e.keyNumber << "/" << (int)e.time << " " << e.value1 << ", " << e.value2 << "]";
  return out;
}


} // ml
