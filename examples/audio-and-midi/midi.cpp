
// example of reading MIDI messages with MIDIInput.

#include "madronalib.h"

using namespace ml;

int main( int argc, char *argv[] )
{
  // Make and start the Timers. Do this once in an application.
  bool deferToMainThread = false;
  SharedResourcePointer<ml::Timers> t;
  t->start(deferToMainThread);
  
  // Define our message handler function.
  const auto& handleMsg = [](MIDIMessage m)->void
  {
    std::cout << "handleMsg got " << m.size() << "bytes: ";
    for ( int i=0; i<m.size(); i++ )
      std::cout << (int)m[i] << " ";
    std::cout << "\n";
  };

  // make a MIDI input and start handling incoming messages with our function.
  MIDIInput midiInput;
  if(midiInput.start(handleMsg))
  {
    std::cout << "Reading MIDI from API " << midiInput.getAPIDisplayName() << ", port " << midiInput.getPortName() << " ...\n";
    while(true) {
      std::this_thread::sleep_for(milliseconds(1000));
    }
  }
  else
  {
    std::cout << "MIDI input not started.\n";
  }
  return 0;
}
