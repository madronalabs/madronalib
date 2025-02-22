// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// example of RtAudio wrapping low-level madronalib DSP code.

#include "MLAudioProcessor.h"

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

class ParamsExampleProcessor :
  public AudioProcessor
{
  // sine generators.
  SineGen s1, s2;
  
public:
  ParamsExampleProcessor(size_t nInputs, size_t nOutputs, int sampleRate) :
    AudioProcessor(nInputs, nOutputs, sampleRate) {}
  
  // SignalProcessor implementation
  
  // declare the processVector function that will run our DSP in vectors of size kFloatsPerDSPVector
  // with the nullptr constructor argument above, AudioProcessor
  void processVector(MainInputs inputs, MainOutputs outputs, void *stateDataUnused) override
  {
    // get params from the SignalProcessor.
    float f1 = getRealFloatParam("freq1");
    float f2 = getRealFloatParam("freq2");
    
    // Running the sine generators makes DSPVectors as output.
    // The input parameter is omega: the frequency in Hz divided by the sample rate.
    // The output sines are multiplied by the gain.
    outputs[0] = s1(f1/kSampleRate)*kOutputGain;
    outputs[1] = s2(f2/kSampleRate)*kOutputGain;
  }
};

int main()
{
  // The AudioProcessor object adapts the RtAudio loop to our buffered processing and runs the example.
  ParamsExampleProcessor exampleProc(kInputChannels, kOutputChannels, kSampleRate);
  
  // the processor can use a temporary ParameterDescriptionList here.
  ParameterDescriptionList pdl;
  
  // read parameter descriptions into list
  readParameterDescriptions(pdl);
  
  // build the stored parameter tree, creating descriptions and projections
  exampleProc.buildParams(pdl);
  
  exampleProc.setDefaultParams();
  
  // set a parameter of the processor as a normalized value.
  // if not set, parameters begin at their default values.
  exampleProc.setParamFromNormalizedValue("freq2", 0.6);

  return exampleProc.run();
}

