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
namespace llllpluginnamellll {

FUID PluginProcessor::uid(0xBBBBBBBB, 0xBBBBBBBB, 0xBBBBBBBB, 0xBBBBBBBB);

PluginProcessor::PluginProcessor()
{
  // register its editor class(the same than used in againentry.cpp)
	setControllerClass(PluginController::uid);
}

PluginProcessor::~PluginProcessor()
{
}

tresult PLUGIN_API PluginProcessor::setProcessing (TBool state)
{
  //debug << "setProcessing: " << (state > 0 ? "true" : "false") << "\n";
  if(state == true)
  {
    audioContext->clear();
  }
  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::initialize(FUnknown* context)
{
  //---always initialize the parent-------
  tresult result = AudioEffect::initialize(context);

  if(result != kResultOk)
  {
    return result;
  }
  
  //---create Audio In/Out buses------
  // we want a stereo Input and a Stereo Output
  addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  
  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::terminate()
{
  return AudioEffect::terminate();
}

tresult PLUGIN_API PluginProcessor::setActive(TBool state)
{
  return AudioEffect::setActive(state);
}

tresult PLUGIN_API PluginProcessor::process(ProcessData& data)
{
  processParameterChanges(data.inputParameterChanges);
  processSignals(data);
  return kResultTrue;
}

tresult PLUGIN_API PluginProcessor::setState(IBStream* state)
{
  // called when we load a preset, the model has to be reloaded
  
  IBStreamer streamer(state, kLittleEndian);
  float savedGain = 0.f;
  if(streamer.readFloat(savedGain) == false)
    return kResultFalse;
  
  float savedGainReduction = 0.f;
  if(streamer.readFloat(savedGainReduction) == false)
    return kResultFalse;
  
  int32 savedBypass = 0;
  if(streamer.readInt32(savedBypass) == false)
    return kResultFalse;
  
  fGain = savedGain;
  fGainReduction = savedGainReduction;
  bBypass = savedBypass > 0;
  
  // Example of using the IStreamAttributes interface
  FUnknownPtr<IStreamAttributes> stream(state);
  if(stream)
  {
    IAttributeList* list = stream->getAttributes();
    if(list)
    {
      // get the current type(project/Default..) of this state
      String128 string = {0};
      if(list->getString(PresetAttributes::kStateType, string, 128 * sizeof(TChar)) ==
          kResultTrue)
      {
        UString128 tmp(string);
        char ascii[128];
        tmp.toAscii(ascii, 128);
        if(!strncmp(ascii, StateType::kProject, strlen(StateType::kProject)))
        {
          // we are in project loading context...
        }
      }
      
      // get the full file path of this state
      TChar fullPath[1024];
      memset(fullPath, 0, 1024 * sizeof(TChar));
      if(list->getString(PresetAttributes::kFilePathStringType, fullPath,
                           1024 * sizeof(TChar)) == kResultTrue)
      {
        // here we have the full path ...
      }
    }
  }
  
  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::getState(IBStream* state)
{
  // here we need to save the model
  IBStreamer streamer(state, kLittleEndian);
  streamer.writeFloat(fGain);
  streamer.writeFloat(fGainReduction);
  streamer.writeInt32(bBypass ? 1 : 0);
  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::setupProcessing(ProcessSetup& newSetup)
{
  // called before the process call, always in a disable state(not active)
  // here we could keep a trace of the processing mode(offline,...) for example.
  // currentProcessMode = newSetup.processMode;
  
  _sampleRate = newSetup.sampleRate;
  
  audioContext = std::make_unique< AudioContext >(kInputChannels, kOutputChannels, _sampleRate);
  processBuffer = std::make_unique< SignalProcessBuffer >(kInputChannels, kOutputChannels, kMaxHostBlockSize);
  
  return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API PluginProcessor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
  if(numIns)
  {
    if(SpeakerArr::getChannelCount(inputs[0]) != 2)
      return kResultFalse;
  }
  if(numOuts)
  {
    if(SpeakerArr::getChannelCount(outputs[0]) != 2)
      return kResultFalse;
  }
  return kResultTrue;
}

tresult PLUGIN_API PluginProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
  if(symbolicSampleSize == kSample32)
    return kResultTrue;
  
  return kResultFalse;
}

tresult PLUGIN_API PluginProcessor::notify(IMessage* message)
{
  // we could respond to messages here
  return AudioEffect::notify(message);
}

// --------------------------------------------------------------------------------
// private implementation

bool PluginProcessor::processParameterChanges(IParameterChanges* changes)
{
  if(changes)
  {
    int32 numParamsChanged = changes->getParameterCount();
    // for each parameter which are some changes in this audio block:
    for(int32 i = 0; i < numParamsChanged; i++)
    {
      IParamValueQueue* paramQueue = changes->getParameterData(i);
      if(paramQueue)
      {
        ParamValue value;
        int32 sampleOffset;
        int32 numPoints = paramQueue->getPointCount();
        switch(paramQueue->getParameterId())
        {
          case PluginController::kGainId:
            // we use in this example only the last point of the queue.
            // in some wanted case for specific kind of parameter it makes sense to
            // retrieve all points and process the whole audio block in small blocks.
            if(paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
            {
              fGain =(float)value;
            }
            break;
            
          case PluginController::kBypassId:
            if(paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
            {
              bBypass =(value > 0.5f);
            }
            break;
        }
      }
    }
  }
  return false;
}


void PluginProcessor::effectExampleProcessVector(MainInputs inputs, MainOutputs outputs)
{
  // Running the sine generators makes DSPVectors as output.
  // The input parameter is omega: the frequency in Hz divided by the sample rate.
  // The output sines are multiplied by the gain.
  auto sineL = s1(220.f/_sampleRate)*fGain;
  auto sineR = s2(275.f/_sampleRate)*fGain;
  
  if(bBypass)
  {
    outputs[0] = 0.f;
    outputs[1] = 0.f;
  }
  else
  {
    outputs[0] = sineL;
    outputs[1] = sineR;
  }
}

void effectExampleProcessVectorFn(AudioContext* ctx, void* untypedState)
{
  PluginProcessor* pProc = static_cast< PluginProcessor* >(untypedState);
  return pProc->effectExampleProcessVector(ctx->inputs, ctx->outputs);
};


// ProcessSignals() adapts the VST process() call with its arbitrary frame size to madronalib's
// fixed vector size processing.

void PluginProcessor::processSignals(ProcessData& data)
{
  if(data.numInputs == 0 || data.numOutputs == 0)
  {
    // nothing to do
    return;
  }
  
  // our outputs are not silent
  data.outputs[0].silenceFlags = 0;
  
  // cast I/O pointers: necessary ugliness due to VST's use of void*
  void** in = getChannelBuffersPointer(processSetup, data.inputs[0]);
  void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
  const float** inputs = const_cast<const float **>(reinterpret_cast<float**>(in));
  float** outputs = reinterpret_cast<float**>(out);
  
  assert(processSetup.symbolicSampleSize == kSample32);
  
  // mark our outputs has not silent
  data.outputs[0].silenceFlags = 0;
  
  // run buffered processing
  processBuffer->process(inputs, outputs, data.numSamples, audioContext.get(), effectExampleProcessVectorFn, this);
}


}}} // namespaces

