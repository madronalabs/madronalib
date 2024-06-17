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

//------------------------------------------------------------------------
// GainParameter Declaration
// example of custom parameter(overwriting to and fromString)

class GainParameter : public Parameter
{
public:
  GainParameter(int32 flags, int32 id);
  
  void toString(ParamValue normValue, String128 string) const SMTG_OVERRIDE;
  bool fromString(const TChar* string, ParamValue& normValue) const SMTG_OVERRIDE;
};

//------------------------------------------------------------------------
// GainParameter Implementation

GainParameter::GainParameter(int32 flags, int32 id)
{
  Steinberg::UString(info.title, USTRINGSIZE(info.title)).assign(USTRING("Gain"));
  Steinberg::UString(info.units, USTRINGSIZE(info.units)).assign(USTRING("dB"));
  
  info.flags = flags;
  info.id = id;
  info.stepCount = 0;
  info.defaultNormalizedValue = 0.5f;
  info.unitId = kRootUnitId;
  
  setNormalized(1.f);
}

void GainParameter::toString(ParamValue normValue, String128 string) const
{
  char text[32];
  if(normValue > 0.0001)
  {
    sprintf(text, "%.2f", 20 * log10f((float)normValue));
  }
  else
  {
    strcpy(text, "-oo");
  }
  
  Steinberg::UString(string, 128).fromAscii(text);
}

bool GainParameter::fromString(const TChar* string, ParamValue& normValue) const
{
  Steinberg::UString wrapper((TChar*)string, 128); // don't know buffer size here!
  double tmp = 0.0;
  if(wrapper.scanFloat(tmp))
  {
    // allow only values between -oo and 0dB
    if(tmp > 0.0)
    {
      tmp = -tmp;
    }
    
    normValue = expf(logf(10.f) *(float)tmp / 20.f);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// PluginController implementation

PluginController::PluginController()
{
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
  
  //---Gain parameter--
  auto* gainParam = new GainParameter(ParameterInfo::kCanAutomate, kGainId);
  parameters.addParameter(gainParam);
  
  //---Bypass parameter---
  int32 stepCount = 1;
  ParamValue defaultVal = 0;
  int32 flags = ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass;
  int32 tag = kBypassId;
  parameters.addParameter(STR16("Bypass"), nullptr, stepCount, defaultVal, flags, tag);
  
  return result;
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
  // we receive the current state of the component(processor part)
  // we read only the gain and bypass value...
  if(!state)
    return kResultFalse;
  
  IBStreamer streamer(state, kLittleEndian);
  float savedGain = 0.f;
  if(streamer.readFloat(savedGain) == false)
    return kResultFalse;
  setParamNormalized(kGainId, savedGain);
  
  // jump the GainReduction
  streamer.seek(sizeof(float), kSeekCurrent);
  
  int32 bypassState = 0;
  if(streamer.readInt32(bypassState) == false)
    return kResultFalse;
  setParamNormalized(kBypassId, bypassState ? 1 : 0);
  
  return kResultOk;
}


}}} // namespaces
