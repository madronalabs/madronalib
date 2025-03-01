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

namespace ml
{
// MIDIInput

struct MIDIInput::Impl
{
  std::unique_ptr< RtMidiIn > midiIn;
  std::vector< unsigned char > inputMessage;
  Timer inputTimer;
  uint32_t midiPort{0};
};

MIDIInput::MIDIInput() : pImpl(std::make_unique<Impl>()) {}

bool MIDIInput::start(MIDIMessageHandler handler)
{
  constexpr int kTimerInterval{10};

  int OK{true};
  try
  {
    pImpl->midiIn = std::make_unique<RtMidiIn>();
  }
  catch (RtMidiError& error)
  {
    error.printMessage();
    OK = false;
  }

  if (OK)
  {
    try
    {
      pImpl->midiIn->openPort(pImpl->midiPort);  // just use first port - TEMP
    }
    catch (RtMidiError& error)
    {
      error.printMessage();
      OK = false;
    }
  }

  // Don't ignore sysex, timing, or active sensing messages.
  pImpl->midiIn->ignoreTypes(false, false, false);

  if (OK)
  {
    pImpl->inputTimer.start([&]() { readNewMessages(handler); }, milliseconds(kTimerInterval));
  }
  else
  {
    pImpl->midiIn = nullptr;
  }

  return OK;
}

void MIDIInput::stop()
{
  pImpl->inputTimer.stop();
  if (pImpl->midiIn.get())
  {
    pImpl->midiIn->closePort();
  }
}

// read any new messages from RtMidi and handle them.
void MIDIInput::readNewMessages(const MIDIMessageHandler& handler) {
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
  stop();
}

// free functions

int messageStatus(const MIDIMessage& m) { return (m[0] & 0x70) >> 4; }
int messageChannel(const MIDIMessage& m) { return (m[0] & 0x0f) + 1; }
int messageByte2(const MIDIMessage& m) { return m[1] & 0x7f; }
int messageByte3(const MIDIMessage& m) { return m[2] & 0x7f; }
float toValue(int messageData) { return messageData / 127.0f; }
float messagePitchBendValue(const MIDIMessage& m)
{
  constexpr int offset = 0x2000;
  constexpr float scale = 1.f / float(0x3FFF);
  int loByte =  m[1] & 0x7f;
  int hiByte = m[2] & 0x7f;
  int bothBytes = (hiByte << 7) & loByte;
  return float(bothBytes - offset) * scale;
}

Event MIDIMessageToEvent(const MIDIMessage& m)
{
  Event e;
  e.channel = messageChannel(m);
  int status = messageStatus(m);
  switch (status)
  {
    case kMIDINoteOff:
    {
      e.type = kNoteOff;
      e.keyNumber = messageByte2(m);
      e.value1 = toValue(messageByte3(m));
      break;
    }
    case kMIDINoteOn:
    {
      e.type = kNoteOn;
      e.keyNumber = messageByte2(m);
      e.value1 = toValue(messageByte3(m));
      break;
    }
    case kMIDIPolyPressure:
    {
      e.type = kNotePressure;
      e.keyNumber = messageByte2(m);
      e.value1 = toValue(messageByte3(m));
      break;
    }
    case kMIDIControlChange:
    {
      e.type = kController;
      e.keyNumber = messageByte2(m);
      e.value1 = toValue(messageByte3(m));
      e.value2 = e.keyNumber;
      break;
    }
    case kMIDIProgramChange:
    {
      e.type = kProgramChange;
      e.keyNumber = messageByte2(m);
      break;
    }
    case kMIDIChannelPressure:
    {
      e.type = kChannelPressure;
      e.value1 = toValue(messageByte2(m));
      break;
    }
    case kMIDIPitchBend:
    {
      e.type = kPitchBend;
      e.value1 = messagePitchBendValue(m);
      break;
    }
    default:
    {
      e.type = kNull;
      break;
    }
  }
  return e;
}
} // ml