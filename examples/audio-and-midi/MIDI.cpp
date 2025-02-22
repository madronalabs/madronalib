
#include "madronalib.h"

using namespace ml;

int main( int argc, char *argv[] )
{
  MIDIInput midiInput;
  if (midiInput.init()) {
    midiInput.start();
  }

  // start the Timers. call this once in an application.
  bool deferToMainThread = false;
  SharedResourcePointer<ml::Timers> t;
  t->start(deferToMainThread);

  std::cout << "Reading MIDI from API " << midiInput.getAPIDisplayName() << ", port " << midiInput.getPortName() << " ...\n";

  while(true) {
    std::this_thread::sleep_for(milliseconds(2000));
  }
  return 0;
}
