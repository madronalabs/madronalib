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
#include "madronalib/MLSynthInput.h"

using namespace ml;

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

constexpr int kMaxProcessBlockFrames = 4096;
constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;

//-----------------------------------------------------------------------------
class PluginProcessor : public AudioEffect, public SignalProcessor
{
  friend void PluginProcessorProcessVectorFn(MainInputs ins, MainOutputs outs, void* state);
  
public:
  static FUnknown* createInstance(void*) { return(IAudioProcessor*)new PluginProcessor; }
  static FUID uid;

  PluginProcessor();
  ~PluginProcessor();

  // from ComponentBase
  tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;
  
  // AudioEffect interface
  tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate() SMTG_OVERRIDE;
  tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
  tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;
  tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup) SMTG_OVERRIDE;
  tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;
  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;
  
private:

  void synthProcessVector(MainInputs inputs, MainOutputs outputs);

  // process VST parameter changes.
  bool processParameterChanges(IParameterChanges* changes);
  
  // send all MIDI events to the SynthInput object.
  void processEvents (IEventList* events);
 
  // This function does all the signal processing in the plugin.
  void processSignals(ProcessData& data);
  
  
  
  std::unique_ptr< SynthInput > _synthInput;
  
  float fGain{1.f};
  float fGainReduction{0.f};
  bool bBypass {false};
    
  float _sampleRate{0.f};
  
  // sine generators.
  SineGen s1, s2;
};

}}} // namespaces

