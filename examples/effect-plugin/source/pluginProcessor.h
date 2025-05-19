// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "madronalib/mldsp.h"
#include "madronalib/madronalib.h"
#include "madronalib/MLPlatform.h"
#include "madronalib/MLSignalProcessor.h"

using namespace ml;

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

constexpr int kMaxHostBlockSize{16384};
constexpr int kInputChannels = 2;
constexpr int kOutputChannels = 2;

//-----------------------------------------------------------------------------
class PluginProcessor : public AudioEffect, public SignalProcessor
{

public:
  static FUnknown* createInstance(void*) { return(IAudioProcessor*)new PluginProcessor; }
  static FUID uid;

  PluginProcessor();
  ~PluginProcessor();

  // from ComponentBase
  tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;

  // AudioEffect interface
  tresult PLUGIN_API setProcessing (TBool state) SMTG_OVERRIDE;
  tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate() SMTG_OVERRIDE;
  tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
  tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;
  tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup) SMTG_OVERRIDE;
  tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;
  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;

  
  // declare the processVector function that will run our DSP in vectors of size kFloatsPerDSPVector
  void effectExampleProcessVector(MainInputs inputs, MainOutputs outputs);

private:
  
  // unlike the madronalib examples, we don't use an AudioTask. So we need our own buffer
  // and AudioContext.
  std::unique_ptr<SignalProcessBuffer> processBuffer;
  std::unique_ptr<AudioContext> audioContext;
  
  // process VST parameter changes.
  bool processParameterChanges(IParameterChanges* changes);
  
  // This function does all the signal processing in the plugin.
  void processSignals(ProcessData& data);
  
  float fGain{1.f};
  float fGainReduction{0.f};
  bool bBypass {false};
  
  float _sampleRate{0.f};
  
  // sine generators.
  SineGen s1, s2;
};

}}} // namespaces

