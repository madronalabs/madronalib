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
#include "madronalib/MLEventsToSignals.h"
#include "madronalib/MLDSPFilters.h"

using namespace ml;

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

constexpr int kMaxProcessBlockFrames = 4096;
constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr size_t kMaxVoices = 4;

// plugin-specific parameter IDs
enum
{
  kBypassId,
  kCutoffId,
  kAttackId,
  kDecayId,
  kSustainId,
  kReleaseId,
  kNumPluginParameters
};

static constexpr int kMIDIParamsStart{kNumPluginParameters};
static constexpr int kVST3MIDIChannels{16};
static constexpr int kVST3MIDICCParams{128};
static constexpr int kVST3MIDISpecialParams{2};
static constexpr int kVST3MIDIParamsPerChannel{kVST3MIDICCParams + kVST3MIDISpecialParams};
static constexpr int kVST3MIDITotalParams = kVST3MIDIChannels*kVST3MIDIParamsPerChannel;

//-----------------------------------------------------------------------------
class PluginProcessor : public AudioEffect, public SignalProcessor
{
  friend void PluginProcessorProcessVectorFn(MainInputs ins, MainOutputs outs, void* state);
  
public:
  static FUnknown* createInstance(void*) { return(IAudioProcessor*)new PluginProcessor; }
  static FUID uid;

  PluginProcessor();
  ~PluginProcessor() = default;

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
  
  // send all MIDI events to the EventsToSignals object.
  void processEvents (IEventList* events);

  // This function does all the signal processing in the plugin.
  void processSignals(ProcessData& data);
  
  void debugStuff();
  
  // the EventsToSignals processes incoming MIDI events and allocates synth voices.
  std::unique_ptr< EventsToSignals > _synthInput;

  // parameters
  // to keep this example very simple we use parameters in their default range of 0-1
  bool bBypass {false};
  float fCutoff{0.5f};
  float fAttack{0.5f};
  float fDecay{0.5f};
  float fSustain{0.5f};
  float fRelease{0.5f};
  bool newEnvParams{true};

  float _sampleRate{0.f}; // in Hz, not reciprocal
  int _debugCounter{0};
  
  class SynthVoice
  {
  public:
    
    // process pitch and amp signals and return stereo output
    DSPVectorArray< 2 > processVector(DSPVector pitch, DSPVector vel, DSPVector cutoff, float sr, bool debug);
    
    void setEnvParams(float a, float d, float s, float r, float sr);
    
    void clear()
    {
      osc1.clear();
      filt1.clear();
      env1.clear();
    }
    
    // oscillator
    SawGen osc1;
    
    // filter
    Lopass filt1;
    
    // envelope
    ADSR env1;
  };
  
  LinearGlide _cutoffGlide;
  Scale defaultScale;
  std::array< SynthVoice, kMaxVoices > _voices;
};

}}} // namespaces

