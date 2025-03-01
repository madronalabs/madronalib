//
// Created by Randy Jones on 2/26/25.
//

// example running MIDI and generating audio from controllers.

#include "madronalib.h"
#include "MLAudioTask.h"

using namespace ml;

// TEMP
constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kOutputGain = 0.1f;

// TODO EventsToSignals goes into some new SignalProcessingContext type object
struct ExampleCallbackData {
  AudioTask* rtProc{nullptr};
  std::vector< SineGen > sineGens;
};

// processAudio() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
void processAudio(MainInputs inputs, MainOutputs outputs, void *pData)
{
  ExampleCallbackData* cdata = reinterpret_cast<ExampleCallbackData*>(pData);


  // now do fun example stuff

}

int main( int argc, char *argv[] )
{
  // fill a struct with the data the callback will need to create a context.
  ExampleCallbackData vcData{nullptr};

  // The AudioProcessor object adapts the RtAudio loop to our buffered processing and runs the audio stream.
  AudioTask vmExampleProc(kInputChannels, kOutputChannels, kSampleRate, processAudio, &vcData);

  // TODO unfuck this
  vcData.rtProc = &vmExampleProc;

  const auto& handleMIDI = [&](MIDIMessage m)->void
  {
    std::cout << "handleMsg got " << m.size() << "bytes: ";
    for ( int i=0; i<m.size(); i++ )
      std::cout << std::hex<< (int)m[i] << std::dec << " ";
    std::cout << "\n";


    auto* audioProcessor = &vmExampleProc;
    vmExampleProc.eventsToSignals.addEvent(MIDIMessageToEvent(m));

  };

  // start the MIDI Input.
  MIDIInput midiInput;
  if (!midiInput.start(handleMIDI)) {
    std::cout << "couldn't start MIDI input!\n";
    return 0;
  }

  // start the Timers. call this once in an application.
  bool deferToMainThread = false;
  SharedResourcePointer<ml::Timers> t;
  t->start(deferToMainThread);


  // TODO don't require setting sample rate twice
  vmExampleProc._currentTime.setTimeAndRate(0, 120.0, true, kSampleRate);
  vmExampleProc.startAudio();

  while(true) {
    std::this_thread::sleep_for(milliseconds(2000));
    std::cout << "samplesSinceStart: " << vmExampleProc._currentTime.samplesSinceStart << "\n";
  }



  return 0;
}
