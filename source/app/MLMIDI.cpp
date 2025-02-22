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

bool MIDIInput::init() {
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

  pImpl->messages = std::make_unique< Queue< uint8_t > >( kQueueSize );
  return OK;
}

void MIDIInput::start() {
  int kTimerInterval{1000};
  pImpl->inputTimer.start([&](){read();}, milliseconds(10));
}

void MIDIInput::read() {
  double timeStamp;
  int nBytes{0};
  int counter{0};
  do{
    timeStamp = pImpl->midiIn->getMessage( &pImpl->inputMessage );
    nBytes = pImpl->inputMessage.size();

    // TEMP
    for ( int i=0; i<nBytes; i++ )
      std::cout << "Byte " << i << " = " << (int)pImpl->inputMessage[i] << ", ";
    if ( nBytes > 0 )
      std::cout << "stamp = " << timeStamp << std::endl;

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
