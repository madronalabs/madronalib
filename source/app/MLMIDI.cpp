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
  std::unique_ptr<RtMidiIn> midiIn_;
  std::vector<unsigned char> inputMessage_;
  Timer inputTimer_;
  uint32_t midiPort_{0};

  // read any new messages from RtMidi and handle them with the handler function.
  void readNewMessages(const MIDIMessageHandler& handler)
  {
    double timeStamp;
    size_t nBytes{0};
    int counter{0};

    do
    {
      // when there are no new messages, RtMidiIn::getMessage() returns a message with size 0.
      timeStamp = midiIn_->getMessage(&inputMessage_);
      nBytes = inputMessage_.size();

      if (nBytes > 0)
      {
        handler(inputMessage_);
      }
    } while (nBytes > 0);
  }
};

MIDIInput::MIDIInput() : pImpl(std::make_unique<Impl>()) {}

MIDIInput::~MIDIInput() { stop(); }

bool MIDIInput::start(MIDIMessageHandler handler)
{
  int OK{true};
  try
  {
    pImpl->midiIn_ = std::make_unique<RtMidiIn>();
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
      pImpl->midiIn_->openPort(pImpl->midiPort_);  // just use first port for now - TODO select ports
    }
    catch (RtMidiError& error)
    {
      error.printMessage();
      OK = false;
    }
  }

  // Don't ignore sysex, timing, or active sensing messages.
  pImpl->midiIn_->ignoreTypes(false, false, false);

  // TODO this makes an OK demo but we need to do more work to get messages
  // with accurate timestamps from MIDI to the audio thread
  constexpr int kTimerInterval{1};
  if (OK)
  {
    pImpl->inputTimer_.start([&]() { pImpl->readNewMessages(handler); },
                            milliseconds(kTimerInterval));
  }
  else
  {
    pImpl->midiIn_ = nullptr;
  }

  return OK;
}

void MIDIInput::stop()
{
  pImpl->inputTimer_.stop();
  if (pImpl->midiIn_.get())
  {
    pImpl->midiIn_->closePort();
  }
}

std::string MIDIInput::getAPIDisplayName()
{
  auto& midiin = pImpl->midiIn_;
  return midiin->getApiDisplayName(midiin->getCurrentApi());
}

std::string MIDIInput::getPortName()
{
  auto& midiin = pImpl->midiIn_;
  auto portNum = pImpl->midiPort_;
  return midiin->getPortName(portNum);
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
  int loByte = m[1] & 0x7f;
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
      e.sourceIdx = messageByte2(m);
      e.value1 = toValue(messageByte3(m));
      break;
    }
    case kMIDINoteOn:
    {
      e.type = kNoteOn;
      e.sourceIdx = messageByte2(m);
      e.value1 = toValue(messageByte3(m));  // velocity
      break;
    }
    case kMIDIPolyPressure:
    {
      e.type = kNotePressure;
      e.sourceIdx = messageByte2(m);
      e.value1 = toValue(messageByte3(m));
      break;
    }
    case kMIDIControlChange:
    {
      e.type = kController;
      e.sourceIdx = messageByte2(m);
      e.value1 = toValue(messageByte3(m));
      break;
    }
    case kMIDIProgramChange:
    {
      e.type = kProgramChange;
      e.sourceIdx = messageByte2(m);
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
}  // namespace ml
