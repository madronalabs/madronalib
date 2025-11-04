// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// example of RtAudio wrapping low-level madronalib DSP code.

#include "MLAudioTask.h"

using namespace ml;

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kOutputGain = 0.1f;
constexpr float kFreqLo = 40, kFreqHi = 4000;

inline void readParameterDescriptions(ParameterDescriptionList& params)
{
  // Processor parameters
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "freq1" },
    { "range", { kFreqLo, kFreqHi } },
    { "log", true },
    { "units", "Hz" }
  } ) );
  
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "freq2" },
    { "range", { kFreqLo, kFreqHi } },
    { "log", true },
    { "units", "Hz" }
  } ) );
  
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "gain" },
    { "range", {0, kOutputGain} }
  } ) );
}

// here we inherit from SignalProcessor, which gives us Parameters we can set and get.
class ExampleProcessor : public SignalProcessor
{
public:
  // sine generators.
  SineGen s1, s2;
};

void processParamsExample(AudioContext* ctx, void *untypedState)
{
  auto state = static_cast< ExampleProcessor* >(untypedState);

  // get params from the SignalProcessor.
  float f1 = state->getRealFloatParamX("freq1");
  float f2 = state->getRealFloatParamX("freq2");
    
  // Running the sine generators makes DSPVectors as output.
  // The input parameter is omega: the frequency in Hz divided by the sample rate.
  // The output sines are multiplied by the gain.
  ctx->outputs[0] = state->s1(f1/kSampleRate)*kOutputGain;
  ctx->outputs[1] = state->s2(f2/kSampleRate)*kOutputGain;
};

int main()
{
  ExampleProcessor state;
  AudioContext ctx(kInputChannels, kOutputChannels, kSampleRate);
  AudioTask exampleTask(&ctx, processParamsExample, &state);
  
  // the processor can use a temporary ParameterDescriptionList here.
  ParameterDescriptionList pdl;
  
  // read parameter descriptions into list
  readParameterDescriptions(pdl);
  
  // build the stored parameter tree, creating descriptions and projections
  state.buildParams(pdl);
  state.setDefaultParams();
  
  // set a parameter of the processor as a normalized value.
  // if not set, parameters begin at their default values.
  state.setParamFromNormalizedValue("freq2", 0.6);

  return exampleTask.runConsoleApp();
}


