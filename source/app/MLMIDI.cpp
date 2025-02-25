//
// Created by Randy Jones on 2/21/25.
//

#include <iostream>
#include <cstdlib>
#include <signal.h>

#include "MLMIDI.h"
#include "MLQueue.h"
#include "MLTimer.h"
#include "rtmidi/RtMidi.h"

using namespace ml;

struct MIDIInput::Impl
{
  std::unique_ptr< RtMidiIn > midiIn;
  std::unique_ptr< Queue< uint8_t > > messages;
  std::vector< unsigned char > inputMessage;
  Timer inputTimer;
  uint32_t midiPort{0};
};

MIDIInput::MIDIInput() : pImpl(std::make_unique<Impl>()) {}

bool MIDIInput::start(MIDIMessageHandler handler)
{
  constexpr int kTimerInterval{10};

  int OK{true};
  try {
    pImpl->midiIn = std::make_unique< RtMidiIn >();
  }
  catch ( RtMidiError &error ) {
    error.printMessage();
    OK = false;
  }

  if (OK) {
    try {
      pImpl->midiIn->openPort( pImpl->midiPort ); // just use first port - TEMP
    }
    catch ( RtMidiError &error ) {
      error.printMessage();
      OK = false;
    }
  }

  // Don't ignore sysex, timing, or active sensing messages.
  pImpl->midiIn->ignoreTypes( false, false, false );

  if (OK)
  {
    pImpl->inputTimer.start([&](){readNewMessages(handler);}, milliseconds(kTimerInterval));
  }

  return OK;
}

// read any new messages from RtMidi and handle them.
void MIDIInput::readNewMessages(MIDIMessageHandler handler) {
  double timeStamp;
  int nBytes{0};
  int counter{0};

  do{
    // when there are no new messages, getMessage() returns a message with size 0.
    timeStamp = pImpl->midiIn->getMessage( &pImpl->inputMessage );
    nBytes = pImpl->inputMessage.size();

    if ( nBytes > 0 )
    {
      handler(pImpl->inputMessage);
    }

  } while (nBytes > 0);
}

std::string MIDIInput::getAPIDisplayName()
{
  auto& midiin = pImpl->midiIn;
  return midiin->getApiDisplayName(midiin->getCurrentApi());
}

std::string MIDIInput::getPortName()
{
  auto& midiin = pImpl->midiIn;
  auto portNum = pImpl->midiPort;
  return midiin->getPortName(portNum);
}

MIDIInput::~MIDIInput() {
  pImpl->inputTimer.stop();
}
