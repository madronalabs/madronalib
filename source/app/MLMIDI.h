//
// Created by Randy Jones on 2/21/25.
//

#pragma once

namespace ml
{
// TODO MIDI input generates events
// EventsToSignals makes note and controller signals.
// OSC could also generate events for EventsToSignals
// composition not inheritance!

class MIDIInput {
  static constexpr size_t kQueueSize{1024};

  MIDIInput();
  ~MIDIInput();

  bool init();
  void start();
  void read();

private:
  struct Impl;
  std::unique_ptr< Impl > pImpl;
};
}

