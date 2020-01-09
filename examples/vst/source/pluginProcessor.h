// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "madronalib/mldsp.h"
using namespace ml;

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

constexpr int kMaxProcessBlockFrames = 4096;
constexpr int kInputChannels = 2;
constexpr int kOutputChannels = 2;

//-----------------------------------------------------------------------------
class PluginProcessor : public AudioEffect
{
public:
  static FUnknown* createInstance (void*) { return (IAudioProcessor*)new PluginProcessor; }
  static FUID uid;

  PluginProcessor ();
  ~PluginProcessor ();

  // AudioEffect interface
  tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate () SMTG_OVERRIDE;
  tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
  tresult PLUGIN_API process (ProcessData& data) SMTG_OVERRIDE;
  tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API setupProcessing (ProcessSetup& newSetup) SMTG_OVERRIDE;
  tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;
  tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE;
  tresult PLUGIN_API notify (IMessage* message) SMTG_OVERRIDE;
  
private:
  bool processParameterChanges (IParameterChanges* changes);
  void processSignals (ProcessData& data);
  
  float fGain{1.f};
  float fGainReduction{0.f};
  bool bBypass {false};
  
  // buffer object to call processVectors from process() calls of arbitrary frame sizes
  VectorProcessBuffer<kInputChannels, kOutputChannels, kMaxProcessBlockFrames> processBuffer;

  // declare the processVectors function that will run our DSP in vectors of size kFloatsPerDSPVector
  DSPVectorArray<kOutputChannels> processVectors(const DSPVectorArray<kInputChannels>& inputVectors);
  
  // set up the function parameter for processBuffer.process()
  using processFnType = std::function<DSPVectorArray<kOutputChannels>(const DSPVectorArray<kInputChannels>&)>;
  processFnType processFn{ [&](const DSPVectorArray<kInputChannels> inputVectors) { return processVectors(inputVectors); } };
  
  float _sampleRate{0.f};
  
  // sine generators.
  SineGen s1, s2;

};

}}} // namespaces

