//
// Created by Randy Jones on 2/21/25.
//

#pragma once

#include "MLEvent.h"

namespace ml
{
enum MIDIMessageType
{
  kMIDINoteOff = 0,
  kMIDINoteOn = 1,
  kMIDIPolyPressure = 2,
  kMIDIControlChange = 3,
  kMIDIProgramChange = 4,
  kMIDIChannelPressure = 5,
  kMIDIPitchBend = 6
};

using MIDIMessage = std::vector< unsigned char >;

using MIDIMessageHandler = std::function< void(const MIDIMessage&) >;

class MIDIInput {

public:

  MIDIInput();
  ~MIDIInput();

  bool start(MIDIMessageHandler handler);
  void stop();

  std::string getAPIDisplayName();
  std::string getPortName();

private:
  struct Impl;
  std::unique_ptr< Impl > pImpl;

  void readNewMessages(const MIDIMessageHandler& handler);
};

Event MIDIMessageToEvent(const MIDIMessage& message);

} // namespace ml

