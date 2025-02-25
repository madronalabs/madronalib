//
// Created by Randy Jones on 2/21/25.
//

#pragma once

namespace ml
{
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

  void readNewMessages(MIDIMessageHandler handler);
};
}

