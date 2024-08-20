// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include "pluginController.h"

#include <cmath>
#include <iostream>

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

FUID PluginController::uid(0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA);


//-----------------------------------------------------------------------------
// PluginController implementation

PluginController::PluginController()
{
  std::cout << "PluginController()\n";
}

PluginController::~PluginController()
{
}

tresult PLUGIN_API PluginController::initialize(FUnknown* context)
{
  tresult result = EditController::initialize(context);
  if(result != kResultOk)
  {
    return result;
  }

  int32 paramFlags;
  ParamValue defaultVal = 0;
  int32 stepCount = 1;

  // add a bypass parameter
  paramFlags = ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass;
  parameters.addParameter(STR16("Bypass"), nullptr, stepCount, defaultVal, paramFlags, kBypassId);
  
  // add our plugin-specific parameters
  stepCount = 0;
  paramFlags = ParameterInfo::kCanAutomate;
  parameters.addParameter(STR16("cutoff"), nullptr, stepCount, 0.5, paramFlags, kCutoffId);
  parameters.addParameter(STR16("attack"), nullptr, stepCount, 0.5, paramFlags, kAttackId);
  parameters.addParameter(STR16("decay"), nullptr, stepCount, 0.5, paramFlags, kDecayId);
  parameters.addParameter(STR16("sustain"), nullptr, stepCount, 0.5, paramFlags, kSustainId);
  parameters.addParameter(STR16("release"), nullptr, stepCount, 0.5, paramFlags, kReleaseId);

  // start making new units after any existing ones
  Steinberg::Vst::UnitInfo unitInfo;
  const int kMIDIParamsUnitIDStart{getUnitCount() + 1};
  Steinberg::UString unitNameSetter(unitInfo.name, 128);

  
  for (int chan = 0; chan < kVST3MIDIChannels; chan++)
  {
    int newUnitID = kMIDIParamsUnitIDStart + chan;
    int newParamID{0};
    TextFragment unitNameText("channel", ml::textUtils::naturalNumberToText(chan + 1));
    unitInfo.id = newUnitID;
    unitInfo.parentUnitId = kRootUnitId;
    unitInfo.programListId = Steinberg::Vst::kNoProgramListId;
    unitNameSetter.fromAscii(unitNameText.getText());
    addUnit(new Steinberg::Vst::Unit(unitInfo));
    
    // because VST3 tries to insulate the plugin from MIDI, we do this horrible hack
    // of creating and mapping many parameters to get MIDI CC and Channel from the Param ID.

    // add 128 MIDI CCs
    Steinberg::Vst::String128 paramName;
    for (int i = 0; i < kVST3MIDICCParams; i++)
    {
      TextFragment ccStr("cc", ml::textUtils::naturalNumberToText(i + 1));
      Steinberg::UString(paramName, str16BufferSize(Steinberg::Vst::String128)).assign(ccStr.getText());
      newParamID = kMIDIParamsStart + chan*kVST3MIDIParamsPerChannel + i;
      parameters.addParameter(paramName, STR16(""), 0, 0, 0, newParamID, newUnitID);
    }
    
    // add special params
    newParamID = kMIDIParamsStart + chan*kVST3MIDIParamsPerChannel + kVST3MIDICCParams;
    parameters.addParameter(STR16("Channel Aftertouch"), STR16(""), 0, 0, 0, newParamID, newUnitID);
    newParamID = kMIDIParamsStart + chan*kVST3MIDIParamsPerChannel + kVST3MIDICCParams + 1;
    parameters.addParameter(STR16("Pitch Bend"), STR16(""), 0, 0.5, 0, newParamID, newUnitID);
  }
  
  return result;
}

tresult PLUGIN_API PluginController::getMidiControllerAssignment (int32 busIndex, int16 midiChannel, CtrlNumber midiControllerNumber, ParamID& tag)
{
  if(busIndex != 0) { return false; }
  if(!within((int)midiChannel, 0, kVST3MIDIChannels)) { return false; }
  if(!within((int)midiControllerNumber, 0, (int)kCountCtrlNumber)) { return false; }

  tag = kMIDIParamsStart + (midiChannel * kCountCtrlNumber) + midiControllerNumber;
  return kResultTrue;
}

tresult PLUGIN_API PluginController::terminate()
{
	return EditController::terminate();
}

tresult PLUGIN_API PluginController::notify(IMessage* message)
{
  return EditController::notify(message);
}

tresult PLUGIN_API PluginController::setComponentState(IBStream* state)
{
  if(!state)
    return kResultFalse;
  
  IBStreamer streamer(state, kLittleEndian);
  
  // bypass
  int32 bypassState{0};
  if(streamer.readInt32(bypassState) == false)
    return kResultFalse;
  setParamNormalized(kBypassId, bypassState ? 1 : 0);
  
  // other params
  // float cutoff, attack, decay, sustain, release;

  float paramVal;
  for(int idx=kCutoffId; idx < kNumPluginParameters; ++idx)
  {
    if(streamer.readFloat(paramVal) != false)
    {
      setParamNormalized(idx, paramVal);
    }
    else
    {
      return kResultFalse;
    }
  }

  return kResultOk;
}


}}} // namespaces
