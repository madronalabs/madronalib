// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#include "pluginProcessor.h"
#include "pluginController.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstpresetkeys.h"  // for use of IStreamAttributes
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include <cmath>
#include <cstdlib>
#include <math.h>
#include <iostream>

namespace Steinberg {
namespace Vst {
namespace ml {

//-----------------------------------------------------------------------------
FUID TrackerProcessor::uid (0x61EA12AB, 0xC25447EA, 0xABD8D344, 0xB21A7B40);

//-----------------------------------------------------------------------------
TrackerProcessor::TrackerProcessor ()
{
  // register its editor class (the same than used in againentry.cpp)
	setControllerClass (TrackerController::uid);
}

TrackerProcessor::~TrackerProcessor ()
{
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API TrackerProcessor::initialize (FUnknown* context)
{
  //---always initialize the parent-------
  tresult result = AudioEffect::initialize (context);

  if (result != kResultOk)
  {
    return result;
  }
  
  //---create Audio In/Out buses------
  // we want a stereo Input and a Stereo Output
  addAudioInput (STR16 ("Stereo In"), SpeakerArr::kStereo);
  addAudioOutput (STR16 ("Stereo Out"), SpeakerArr::kStereo);
  
  return kResultOk;
}

tresult PLUGIN_API TrackerProcessor::terminate ()
{
  return AudioEffect::terminate ();
}

tresult PLUGIN_API TrackerProcessor::setActive (TBool state)
{
  return AudioEffect::setActive (state);
}

tresult PLUGIN_API TrackerProcessor::process (ProcessData& data)
{
  processParameterChanges (data.inputParameterChanges);
  
  // TODO for instruments
  // processEvents (data.inputEvents);
  
  processSignals(data);
  
  return kResultTrue;
}


tresult PLUGIN_API TrackerProcessor::setState (IBStream* state)
{
  // called when we load a preset, the model has to be reloaded
  
  IBStreamer streamer (state, kLittleEndian);
  float savedGain = 0.f;
  if (streamer.readFloat (savedGain) == false)
    return kResultFalse;
  
  float savedGainReduction = 0.f;
  if (streamer.readFloat (savedGainReduction) == false)
    return kResultFalse;
  
  int32 savedBypass = 0;
  if (streamer.readInt32 (savedBypass) == false)
    return kResultFalse;
  
  fGain = savedGain;
  fGainReduction = savedGainReduction;
  bBypass = savedBypass > 0;
  
  // Example of using the IStreamAttributes interface
  FUnknownPtr<IStreamAttributes> stream (state);
  if (stream)
  {
    IAttributeList* list = stream->getAttributes ();
    if (list)
    {
      // get the current type (project/Default..) of this state
      String128 string = {0};
      if (list->getString (PresetAttributes::kStateType, string, 128 * sizeof (TChar)) ==
          kResultTrue)
      {
        UString128 tmp (string);
        char ascii[128];
        tmp.toAscii (ascii, 128);
        if (!strncmp (ascii, StateType::kProject, strlen (StateType::kProject)))
        {
          // we are in project loading context...
        }
      }
      
      // get the full file path of this state
      TChar fullPath[1024];
      memset (fullPath, 0, 1024 * sizeof (TChar));
      if (list->getString (PresetAttributes::kFilePathStringType, fullPath,
                           1024 * sizeof (TChar)) == kResultTrue)
      {
        // here we have the full path ...
      }
    }
  }
  
  return kResultOk;
}

tresult PLUGIN_API TrackerProcessor::getState (IBStream* state)
{
  // here we need to save the model
  
  IBStreamer streamer (state, kLittleEndian);
  
  streamer.writeFloat (fGain);
  streamer.writeFloat (fGainReduction);
  streamer.writeInt32 (bBypass ? 1 : 0);
  
  return kResultOk;
}

tresult PLUGIN_API TrackerProcessor::setupProcessing (ProcessSetup& newSetup)
{
  // called before the process call, always in a disable state (not active)
  // here we could keep a trace of the processing mode (offline,...) for example.
  // currentProcessMode = newSetup.processMode;
  
  return AudioEffect::setupProcessing (newSetup);
}

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

tresult PLUGIN_API TrackerProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
  if (symbolicSampleSize == kSample32)
    return kResultTrue;
  
  // we support double processing
  if (symbolicSampleSize == kSample64)
    return kResultTrue;
  
  return kResultFalse;
}

tresult PLUGIN_API TrackerProcessor::notify (IMessage* message)
{
  // we could respond to messages here
  return AudioEffect::notify (message);
}

// --------------------------------------------------------------------------------
// private implementation


bool TrackerProcessor::processParameterChanges (IParameterChanges* changes)
{
  if (changes)
  {
    int32 numParamsChanged = changes->getParameterCount ();
    // for each parameter which are some changes in this audio block:
    for (int32 i = 0; i < numParamsChanged; i++)
    {
      IParamValueQueue* paramQueue = changes->getParameterData (i);
      if (paramQueue)
      {
        ParamValue value;
        int32 sampleOffset;
        int32 numPoints = paramQueue->getPointCount ();
        switch (paramQueue->getParameterId ())
        {
          case TrackerController::kGainId:
            // we use in this example only the last point of the queue.
            // in some wanted case for specific kind of parameter it makes sense to
            // retrieve all points and process the whole audio block in small blocks.
            if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
            {
              fGain = (float)value;
            }
            break;
            
          case TrackerController::kBypassId:
            if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                kResultTrue)
            {
              bBypass = (value > 0.5f);
            }
            break;
        }
      }
    }
  }
  return false;
}

void TrackerProcessor::processSignals (ProcessData& data)
{
  if (data.numInputs == 0 || data.numOutputs == 0)
  {
    // nothing to do
    return;
  }
  
  // (simplification) we suppose in this example that we have the same input channel count than
  // the output
  int32 numChannels = data.inputs[0].numChannels;
  
  //---get audio buffers----------------
  uint32 sampleFramesSize = getSampleFramesSizeInBytes (processSetup, data.numSamples);
  uint32 frames = data.numSamples;
  
  void** in = getChannelBuffersPointer (processSetup, data.inputs[0]);
  void** out = getChannelBuffersPointer (processSetup, data.outputs[0]);
  
  //---check if silence---------------
  // normally we have to check each channel (simplification)
  if (data.inputs[0].silenceFlags != 0)
  {
    // mark output silence too
    data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
    
    // the Plug-in has to be sure that if it sets the flags silence that the output buffer are
    // clear
    for (int32 i = 0; i < numChannels; i++)
    {
      // do not need to be cleared if the buffers are the same (in this case input buffer are
      // already cleared by the host)
      if (in[i] != out[i])
      {
        memset (out[i], 0, sampleFramesSize);
      }
    }
    
    // nothing to do at this point
    return;
  }
  
  // mark our outputs has not silent
  data.outputs[0].silenceFlags = 0;
  
  
  //---in bypass mode outputs should be like inputs-----
  if (bBypass)
  {
    for (int32 i = 0; i < numChannels; i++)
    {
      // do not need to be copied if the buffers are the same
      if (in[i] != out[i])
      {
        memcpy (out[i], in[i], sampleFramesSize);
      }
    }
  }
  else
  {
    //---apply gain factor----------
    float gain = (fGain - fGainReduction);

    // if the applied gain is nearly zero, we could say that the outputs are zeroed and we set
    // the silence flags.
    if (gain < 0.0000001)
    {
      for (int32 i = 0; i < numChannels; i++)
      {
        memset (out[i], 0, sampleFramesSize);
      }
      data.outputs[0].silenceFlags = (1 << numChannels) - 1; // this will set to 1 all channels
    }
    else
    {
      // in real Plug-in it would be better to do dezippering to avoid jump (click) in gain value
      for (int32 i = 0; i < numChannels; i++)
      {
        float* pSrc = static_cast<float*>(in[i]);
        float* pDest = static_cast<float*>(out[i]);
        for(int i=0; i<frames; ++i)
        {
          pDest[i] = pSrc[i]*gain;
        }
      }
    }
  }
}

}}} // namespaces

