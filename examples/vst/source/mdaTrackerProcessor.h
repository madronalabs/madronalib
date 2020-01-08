// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details


#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

namespace Steinberg {
namespace Vst {
namespace ml {

//-----------------------------------------------------------------------------
class TrackerProcessor : public AudioEffect
{
public:
  static FUnknown* createInstance (void*) { return (IAudioProcessor*)new TrackerProcessor; }
  static FUID uid;

  TrackerProcessor ();
  ~TrackerProcessor ();

  // AudioEffect implementation
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
  
  //-----------------------------------------------------------------------------
  float fGain{1.f};
  float fGainReduction{0.f};
  bool bBypass {false};
};

}}} // namespaces

