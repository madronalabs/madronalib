//
// Created by Randy Jones on 2/26/25.
//

// example running MIDI and generating audio from controllers.

#include "madronalib.h"

using namespace ml;

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kOutputGain = 0.5f;

struct ExampleState {
  // This example will listen to these MIDI controllers on any channel.
  // These are the default mappings of my Akai MIDIMix hardware. Pick any numbers you like.
  std::vector< int > sineControllers {19, 23, 27, 31, 49, 53, 57, 61};
  const int volumeControl{62};
  std::vector< SineGen > sineGens;
};

// processAudio() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
void processAudio(AudioContext* ctx, void *untypedState)
{
  ExampleState* state = reinterpret_cast<ExampleState*>(untypedState);

  // now do fun example stuff
  float sr = ctx->sampleRate;
  DSPVector accum;
  auto ctrlToFreq = projections::unityToLogParam({110.f, 440.f});

  // accumulate sine oscillators
  int nSines = state->sineControllers.size();
  for (int i = 0; i < nSines; ++i)
  {
    int ctrlNum = state->sineControllers[i];
    DSPVector ctrlSig = ctx->eventsToSignals.controllers[ctrlNum].output;
    float freqInHz = ctrlToFreq(ctrlSig[0]);
    DSPVector sineSig = state->sineGens[i](freqInHz/sr);
    accum += sineSig;
  }

  // scale total volume and write context output
  DSPVector volumeSig = ctx->eventsToSignals.controllers[state->volumeControl].output;
  accum *= volumeSig*kOutputGain/nSines;
  ctx->outputs[0] = accum;
  ctx->outputs[1] = accum;
}

int main( int argc, char *argv[] )
{
  ExampleState state;
  AudioContext ctx(kInputChannels, kOutputChannels, kSampleRate);
  AudioTask exampleTask(&ctx, processAudio, &state);

  // set up the state:
  // make a sine generator for each contrller number we listen to
  state.sineGens.resize(state.sineControllers.size());

  // define the MIDI handling callback.
  const auto& handleMIDI = [&](MIDIMessage m)->void
  {
    ctx.eventsToSignals.addEvent(MIDIMessageToEvent(m));
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

  // run the audio task
  return exampleTask.run();
}
