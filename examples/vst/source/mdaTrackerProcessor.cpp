/*
 *  mdaTrackerProcessor.cpp
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

#include "mdaTrackerProcessor.h"
#include "mdaTrackerController.h"

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"

#include <cmath>
#include <cstdlib>
#include "base/source/fstreamer.h"

#include <math.h>
#include <iostream>

namespace Steinberg {
namespace Vst {
namespace ml {

//-----------------------------------------------------------------------------
FUID TrackerProcessor::uid (0x61EA12AB, 0xC25447EA, 0xABD8D344, 0xB21B8B40);

//-----------------------------------------------------------------------------
void TrackerProcessor::allocParameters (int32 _numParams)
{
  if (params)
    return;
  numParams = _numParams;
  params = new ParamValue[numParams];
}

//-----------------------------------------------------------------------------
TrackerProcessor::TrackerProcessor ()
{
	setControllerClass (TrackerController::uid);
	allocParameters (8);
}

//-----------------------------------------------------------------------------
TrackerProcessor::~TrackerProcessor ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerProcessor::initialize (FUnknown* context)
{
	tresult res = AudioEffect::initialize (context);
	if (res == kResultTrue)
	{
		addAudioInput (USTRING("Stereo In"), SpeakerArr::kStereo);
		addAudioOutput (USTRING("Stereo Out"), SpeakerArr::kStereo);

		params[0] = (float)0.00; //Mode
		params[1] = (float)1.00; //Dynamics
		params[2] = (float)1.00; //Mix
		params[3] = (float)0.97; //Tracking
		params[4] = (float)0.50; //Trnspose
		params[5] = (float)0.80; //Maximum Hz
		params[6] = (float)0.50; //Trigger dB
		params[7] = (float)0.50; //Output

	}
	return res;
}


//-----------------------------------------------------------------------------
void TrackerProcessor::setBypass (bool state, int32 sampleOffset)
{
  if (state != bypassState)
  {
    bypassState = state;
  }
}

//-----------------------------------------------------------------------------
void TrackerProcessor::setParameter (ParamID index, ParamValue newValue, int32 sampleOffset)
{
  if (numParams > index)
    params[index] = newValue;
}

bool TrackerProcessor::processParameterChanges (IParameterChanges* changes)
{
  if (changes)
  {
    int32 count = changes->getParameterCount ();
    if (count > 0)
    {
      for (int32 i = 0; i < count; i++)
      {
        IParamValueQueue* queue = changes->getParameterData (i);
        if (!queue)
          continue;
        ParamID paramId = queue->getParameterId ();
        int32 pointCount = queue->getPointCount ();
        int32 sampleOffset;
        ParamValue value;
        queue->getPoint (pointCount - 1, sampleOffset, value);
        if (paramId == TrackerController::kBypassParam)
          setBypass (value >= 0.5, sampleOffset);
        else if (paramId == TrackerController::kPresetParam)
          setCurrentProgramNormalized (value);
        else
          setParameter (paramId, value, sampleOffset);
      }
      return true;
    }
  }
  return false;
}


tresult PLUGIN_API TrackerProcessor::process (ProcessData& data)
{
  if (processParameterChanges (data.inputParameterChanges))
  {
    // recalc internal coeffs from param— TODO only on change
    wet = (float)pow (10.0, 2.0*params[7] - 1.0);
  }
  
  // TODO for instruments
  // processEvents (data.inputEvents);
  
  if (data.numSamples > 0)
  {
    doProcessing (data);
  }
  return kResultTrue;
}


//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerProcessor::terminate ()
{
	return AudioEffect::terminate ();
}

//-----------------------------------------------------------------------------

tresult PLUGIN_API TrackerProcessor::setActive (TBool state)
{
  if (state)
  {
    // dphi = 100.f/getSampleRate (); //initial pitch
  }
  return AudioEffect::setActive (state);
}


//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerProcessor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
  if (numIns)
  {
    if (SpeakerArr::getChannelCount (inputs[0]) != 2)
      return kResultFalse;
  }
  if (numOuts)
  {
    if (SpeakerArr::getChannelCount (outputs[0]) != 2)
      return kResultFalse;
  }
  return kResultTrue;
}


//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerProcessor::setState (IBStream* state)
{
  if (!state)
    return kResultFalse;
  
  IBStreamer streamer (state, kLittleEndian);
  
  uint32 temp;
  streamer.readInt32u (temp); // numParams or Header
  
  if (temp == TrackerController::kMagicNumber)
  {
    // read current Program
    streamer.readInt32u (temp);
    setCurrentProgram (temp);
    
    streamer.readInt32u (temp);
  }
  
  // read each parameter
  for (uint32 i = 0; i < temp, i < numParams; i++)
  {
    streamer.readDouble (params[i]);
  }
  
  // bypass
  streamer.readInt32u (temp);
  bypassState = temp > 0;
  
  return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerProcessor::getState (IBStream* state)
{
  if (!state)
    return kResultFalse;
  
  IBStreamer streamer (state, kLittleEndian);
  
  if (hasProgram ())
  {
    // save header key
    uint32 temp = TrackerController::kMagicNumber;
    streamer.writeInt32u (temp);
    
    // save program
    temp = getCurrentProgram ();
    streamer.writeInt32u (temp);
  }
  
  // save number of parameter
  streamer.writeInt32u (numParams);
  
  // save each parameter
  for (uint32 i = 0; i < numParams; i++)
  {
    streamer.writeDouble (params[i]);
  }
  
  // save bypass
  streamer.writeInt32u (bypassState ? 1 : 0);
  
  return kResultTrue;
}

//-----------------------------------------------------------------------------
void TrackerProcessor::doProcessing (ProcessData& data)
{
	int32 sampleFrames = data.numSamples;
	
	float* in1 = data.inputs[0].channelBuffers32[0];
	float* in2 = data.inputs[0].channelBuffers32[1];
	float* out1 = data.outputs[0].channelBuffers32[0];
	float* out2 = data.outputs[0].channelBuffers32[1];

  for(int i=0; i<sampleFrames; ++i)
  {
    out1[i] = in1[i]*wet;
    out2[i] = in2[i]*wet;
  }
  
  
  // MLTEST
  static uint32_t counter{0};
  counter += sampleFrames;
  if(counter > 44100)
  {
    counter -= 44100;
    std::cout << "param: " << params[7]  << " wet: " << wet << "\n";
  }
  
  
  return;

}

}}} // namespaces

