// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details


#include "mdaParameter.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include "mdaTrackerController.h"

#include <cmath>

namespace Steinberg {
namespace Vst {
namespace ml {


//-----------------------------------------------------------------------------
FUID TrackerController::uid (0xBBF70390, 0x94A848F0, 0xAEE965F6, 0x5DA3D3BA);


//------------------------------------------------------------------------
// GainParameter Declaration
// example of custom parameter (overwriting to and fromString)

class GainParameter : public Parameter
{
public:
  GainParameter (int32 flags, int32 id);
  
  void toString (ParamValue normValue, String128 string) const SMTG_OVERRIDE;
  bool fromString (const TChar* string, ParamValue& normValue) const SMTG_OVERRIDE;
};

//------------------------------------------------------------------------
// GainParameter Implementation

GainParameter::GainParameter (int32 flags, int32 id)
{
  Steinberg::UString (info.title, USTRINGSIZE (info.title)).assign (USTRING ("Gain"));
  Steinberg::UString (info.units, USTRINGSIZE (info.units)).assign (USTRING ("dB"));
  
  info.flags = flags;
  info.id = id;
  info.stepCount = 0;
  info.defaultNormalizedValue = 0.5f;
  info.unitId = kRootUnitId;
  
  setNormalized (1.f);
}

void GainParameter::toString (ParamValue normValue, String128 string) const
{
  char text[32];
  if (normValue > 0.0001)
  {
    sprintf (text, "%.2f", 20 * log10f ((float)normValue));
  }
  else
  {
    strcpy (text, "-oo");
  }
  
  Steinberg::UString (string, 128).fromAscii (text);
}

bool GainParameter::fromString (const TChar* string, ParamValue& normValue) const
{
  String wrapper ((TChar*)string); // don't know buffer size here!
  double tmp = 0.0;
  if (wrapper.scanFloat (tmp))
  {
    // allow only values between -oo and 0dB
    if (tmp > 0.0)
    {
      tmp = -tmp;
    }
    
    normValue = expf (logf (10.f) * (float)tmp / 20.f);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
TrackerController::TrackerController () : sampleRate (44100)
{
  for (int32 i = 0; i < kCountCtrlNumber; i++)
    midiCCParamID[i] = -1;
}


//-----------------------------------------------------------------------------
TrackerController::~TrackerController ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::initialize (FUnknown* context)
{
  tresult result = EditControllerEx1::initialize (context);
  if (result != kResultOk)
  {
    return result;
  }
  
  //--- Create Units-------------
  UnitInfo unitInfo;
  Unit* unit;
  
  // create root only if you want to use the programListId
  /*  unitInfo.id = kRootUnitId;  // always for Root Unit
   unitInfo.parentUnitId = kNoParentUnitId;  // always for Root Unit
   Steinberg::UString (unitInfo.name, USTRINGSIZE (unitInfo.name)).assign (USTRING ("Root"));
   unitInfo.programListId = kNoProgramListId;
   
   unit = new Unit (unitInfo);
   addUnitInfo (unit);*/
  
  // create a unit1 for the gain
  unitInfo.id = 1;
  unitInfo.parentUnitId = kRootUnitId; // attached to the root unit
  
  Steinberg::UString (unitInfo.name, USTRINGSIZE (unitInfo.name)).assign (USTRING ("Unit1"));
  
  unitInfo.programListId = kNoProgramListId;
  
  unit = new Unit (unitInfo);
  addUnit (unit);
  
  //---Create Parameters------------
  
  //---Gain parameter--
  auto* gainParam = new GainParameter (ParameterInfo::kCanAutomate, kGainId);
  parameters.addParameter (gainParam);
  
  gainParam->setUnitID (1);
  
  //---VuMeter parameter---
  int32 stepCount = 0;
  ParamValue defaultVal = 0;
  int32 flags = ParameterInfo::kIsReadOnly;
  int32 tag = kVuPPMId;
  parameters.addParameter (STR16 ("VuPPM"), nullptr, stepCount, defaultVal, flags, tag);
  
  //---Bypass parameter---
  stepCount = 1;
  defaultVal = 0;
  flags = ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass;
  tag = kBypassId;
  parameters.addParameter (STR16 ("Bypass"), nullptr, stepCount, defaultVal, flags, tag);
  
  return result;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::terminate ()
{
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::getParamStringByValue (ParamID tag, ParamValue valueNormalized,
                                                           String128 string)
{
  /* example, but better to use a custom Parameter as seen in GainParameter
   switch (tag)
   {
   case kGainId:
   {
   char text[32];
   if (valueNormalized > 0.0001)
   {
   sprintf (text, "%.2f", 20 * log10f ((float)valueNormalized));
   }
   else
   strcpy (text, "-oo");
   
   Steinberg::UString (string, 128).fromAscii (text);
   
   return kResultTrue;
   }
   }*/
  return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::getParamValueByString (ParamID tag, TChar* string,
                                                           ParamValue& valueNormalized)
{
  /* example, but better to use a custom Parameter as seen in GainParameter
   switch (tag)
   {
   case kGainId:
   {
   Steinberg::UString wrapper ((TChar*)string, -1); // don't know buffer size here!
   double tmp = 0.0;
   if (wrapper.scanFloat (tmp))
   {
   valueNormalized = expf (logf (10.f) * (float)tmp / 20.f);
   return kResultTrue;
   }
   return kResultFalse;
   }
   }*/
  return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::notify (IMessage* message)
{
  if (strcmp (message->getMessageID (), "activated") == 0)
  {
    message->getAttributes ()->getFloat ("SampleRate", sampleRate);
    return kResultTrue;
  }
  return EditControllerEx1::notify (message);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::setComponentState (IBStream* state)
{
  // we receive the current state of the component (processor part)
  // we read only the gain and bypass value...
  if (!state)
    return kResultFalse;
  
  IBStreamer streamer (state, kLittleEndian);
  float savedGain = 0.f;
  if (streamer.readFloat (savedGain) == false)
    return kResultFalse;
  setParamNormalized (kGainId, savedGain);
  
  // jump the GainReduction
  streamer.seek (sizeof (float), kSeekCurrent);
  
  int32 bypassState = 0;
  if (streamer.readInt32 (bypassState) == false)
    return kResultFalse;
  setParamNormalized (kBypassId, bypassState ? 1 : 0);
  
  return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::getMidiControllerAssignment (int32 busIndex, int16 channel,
                                                                   CtrlNumber midiControllerNumber,
                                                                   ParamID& tag /*out*/)
{
  if (busIndex == 0 && midiControllerNumber < kCountCtrlNumber && midiCCParamID[midiControllerNumber] != -1)
  {
    tag = midiCCParamID[midiControllerNumber];
    return kResultTrue;
  }
  return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::queryInterface (const char* iid, void** obj)
{
  QUERY_INTERFACE (iid, obj, IMidiMapping::iid, IMidiMapping)
  return EditControllerEx1::queryInterface (iid, obj);
}



}}} // namespaces
