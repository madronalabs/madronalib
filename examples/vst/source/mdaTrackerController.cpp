/*
 *  mdaTrackerController.cpp
 *  mda-vst3
 *
 *  Created by Arne Scheffler on 6/14/08.
 *
 *  mda VST Plug-ins
 *
 *  Copyright (c) 2008 Paul Kellett
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include "mdaParameter.h"
//#include "helpers.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include "mdaTrackerController.h"

namespace Steinberg {
namespace Vst {
namespace ml {

const TChar TrackerController::kMicroSecondsString[] = {0x00b5, 0x0073, 0x0};

//-----------------------------------------------------------------------------
int32 PLUGIN_API TrackerController::getProgramListCount ()
{
  if (parameters.getParameter (kPresetParam))
    return 1;
  return 0;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::getProgramListInfo (int32 listIndex,
                                                       ProgramListInfo& info /*out*/)
{
  Parameter* param = parameters.getParameter (kPresetParam);
  if (param && listIndex == 0)
  {
    info.id = kPresetParam;
    info.programCount = (int32)param->toPlain (1) + 1;
    UString name (info.name, 128);
    name.fromAscii ("Presets");
    return kResultTrue;
  }
  return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::getProgramName (ProgramListID listId, int32 programIndex,
                                                   String128 name /*out*/)
{
  if (listId == kPresetParam)
  {
    Parameter* param = parameters.getParameter (kPresetParam);
    if (param)
    {
      ParamValue normalized = param->toNormalized (programIndex);
      param->toString (normalized, name);
      return kResultTrue;
    }
  }
  return kResultFalse;
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
  if (!state)
    return kResultFalse;
  
  IBStreamer streamer (state, kLittleEndian);
  
  uint32 temp;
  streamer.readInt32u (temp); // numParams or Header
  
  if (temp == kMagicNumber)
  {
    // read current Program
    streamer.readInt32u (temp);
    Parameter* param = parameters.getParameter (kPresetParam);
    if (param)
      param->setNormalized (param->toNormalized (temp));
    
    // numParams
    streamer.readInt32u (temp);
  }
  
  // read each parameter
  for (uint32 i = 0; i < temp; i++)
  {
    ParamValue value;
    if (streamer.readDouble (value) == false)
      return kResultFalse;
    setParamNormalized (i, value);
  }
  
  // bypass
  uint32 bypassState;
  if (streamer.readInt32u (bypassState) == false)
    return kResultFalse;
  
  Parameter* bypassParam = parameters.getParameter (kBypassParam);
  if (bypassParam)
    bypassParam->setNormalized (bypassState);
  
  return kResultTrue;
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


















//-----------------------------------------------------------------------------
FUID TrackerController::uid (0xBBF731F0, 0x94A848F0, 0xAEE965F6, 0x5DA3D3BA);

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
  tresult res = EditControllerEx1::initialize (context);
  if (res == kResultOk)
  {
    UnitInfo uinfo;
    uinfo.id = kRootUnitId;
    uinfo.parentUnitId = kNoParentUnitId;
    uinfo.programListId = kPresetParam;
    UString name (uinfo.name, 128);
    name.fromAscii ("Root");
    addUnit (new Unit (uinfo));
    
    // add bypass parameter
    IndexedParameter* bypassParam = new IndexedParameter (USTRING ("Bypass"), 0, 1, 0, ParameterInfo::kIsBypass | ParameterInfo::kCanAutomate,
                                                          kBypassParam);
    bypassParam->setIndexString (0, UString128 ("off"));
    bypassParam->setIndexString (1, UString128 ("on"));
    parameters.addParameter (bypassParam);
  
  }
  
	if (res == kResultTrue)
	{
		ParamID pid = 0;
		IndexedParameter* modeParam = new IndexedParameter (USTRING("Mode"), USTRING(""), 4, 0.15, ParameterInfo::kCanAutomate | ParameterInfo::kIsList, pid++);
		modeParam->setIndexString (0, UString128("SINE"));
		modeParam->setIndexString (1, UString128("SQUARE"));
		modeParam->setIndexString (2, UString128("SAW"));
		modeParam->setIndexString (3, UString128("RING"));
		modeParam->setIndexString (4, UString128("EQ"));
		parameters.addParameter (modeParam);
		parameters.addParameter (new ScaledParameter (USTRING("Dynamics"), USTRING("%"), 0, 0.6, ParameterInfo::kCanAutomate, pid++, 0, 100, true));
		parameters.addParameter (new ScaledParameter (USTRING("Mix"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100, true));
		parameters.addParameter (new ScaledParameter (USTRING("Glide"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100, true));
		parameters.addParameter (new ScaledParameter (USTRING("Trnspose"), USTRING("semi"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, -36, 36, true));
		parameters.addParameter (new ScaledParameter (USTRING("Maximum"), USTRING("%"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, 0, 100, true));
		parameters.addParameter (new ScaledParameter (USTRING("Trigger"), USTRING("dB"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, -60, 0, true));
		parameters.addParameter (new ScaledParameter (USTRING("Output"), USTRING("dB"), 0, 0.5, ParameterInfo::kCanAutomate, pid++, -20, 20, true));
	}
	return res;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::terminate ()
{
	return EditControllerEx1::terminate ();
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string)
{
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);

	/*
	UString128 result;
	 	switch (tag)
	 	{
			default:
				return BaseController::getParamStringByValue (tag, valueNormalized, string);
		}
		result.copyTo (string, 128);
		return kResultTrue;*/
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerController::getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized)
{
	// TODO
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
	
	/*
	switch (tag)
		{
			default:
				return BaseController::getParamValueByString (tag, string, valueNormalized);
		}
		return kResultFalse;*/	
}

}}} // namespaces
