// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// example of RtAudio wrapping low-level madronalib DSP code.

#include "MLAudioTask.h"

using namespace ml;

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kOutputGainMax = 0.1f;
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
    { "default", 0.1f },
    { "range", {0, kOutputGainMax} }
  } ) );
}

// here we inherit from SignalProcessor, which gives us Parameters we can set and get.
class ExampleProcessor : public SignalProcessor
{
public:
  // sine generators.
  SineGen s1, s2;
};

void processParamsExample(AudioContext* ctx, void *untypedProcState)
{
  auto proc = static_cast< ExampleProcessor* >(untypedProcState);
  
  // get params from the SignalProcessor using runtime path.
  // it will not allocate, so is audio-thread safe, but will take a bit of time
  // to parse the path.
  std::string runtimePathStr("freq1");
  Path freqPath(runtimePathStr.c_str());
  float f1 = proc->params.getRealFloatValueAtPath(freqPath);
  
  // get params from the SignalProcessor using fast compile-time hashed paths.
  float f2 = proc->params.getRealFloatValue("freq2");
  float gain = proc->params.getRealFloatValue("gain");

  // Running the sine generators makes DSPVectors as output.
  // The input parameter is omega: the frequency in Hz divided by the sample rate.
  // The output sines are multiplied by the gain.
  ctx->outputs[0] = proc->s1(f1/kSampleRate)*gain;
  ctx->outputs[1] = proc->s2(f2/kSampleRate)*gain;
  
  // print debug stuff every second
  static int testCtr{0};
  testCtr += kFloatsPerDSPVector;
  if(testCtr > ctx->getSampleRate())
  {
    testCtr = 0;
    std::cout << "gain: " << gain << "\n";
  }
};

int main()
{
  ExampleProcessor proc;
  AudioContext ctx(kInputChannels, kOutputChannels, kSampleRate);
  AudioTask exampleTask(&ctx, processParamsExample, &proc);
  
  // the processor can use a temporary ParameterDescriptionList here.
  ParameterDescriptionList pdl;
  
  // read parameter descriptions into list
  readParameterDescriptions(pdl);
  
  // build the stored parameter tree, creating descriptions and projections
  proc.buildParams(pdl);
  proc.setDefaultParams();
  
  // set a parameter of the processor as a normalized value.
  // if not set, parameters begin at their default values.
  proc.params.setFromNormalizedValue("freq2", 0.6);

  return exampleTask.runConsoleApp();
}
