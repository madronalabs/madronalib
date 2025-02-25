
// example of reading MIDI messages with MIDIInput.

#include "madronalib.h"

using namespace ml;

int main( int argc, char *argv[] )
{
  const auto& handleMsg = [](MIDIMessage m)->void
  {
    std::cout << "handleMsg got " << m.size() << "bytes: ";
    for ( int i=0; i<m.size(); i++ )
      std::cout << (int)m[i] << " ";
    std::cout << "\n";
  };

  MIDIInput midiInput;
  if (midiInput.start(handleMsg))
  {
    // start the Timers. call this once in an application.
    bool deferToMainThread = false;
    SharedResourcePointer<ml::Timers> t;
    t->start(deferToMainThread);

    std::cout << "Reading MIDI from API " << midiInput.getAPIDisplayName() << ", port " << midiInput.getPortName() << " ...\n";

    while(true) {
      std::this_thread::sleep_for(milliseconds(2000));
    }
  }
  else
  {
    std::cout << "MIDI input not started\n";
  }
  return 0;
}
