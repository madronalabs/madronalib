// VST3 instrument example code for madronalib
// (c) 2023, Madrona Labs LLC, all rights reserved
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

// VST3 implementation

FUID PluginProcessor::uid(0xBBBBBBBB, 0xBBBBBBBB, 0xBBBBBBBB, 0xBBBBBBBB);

PluginProcessor::PluginProcessor()
  : SignalProcessor(kInputChannels, kOutputChannels)
{
  // register its editor class(the same than used in againentry.cpp)
	setControllerClass(PluginController::uid);
}

tresult PLUGIN_API PluginProcessor::initialize(FUnknown* context)
{
  //---always initialize the parent-------
  tresult result = AudioEffect::initialize(context);

  if(result != kResultOk)
  {
    return result;
  }
  
  addEventInput (STR16 ("Events In"), 1);
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
  // process parameter changes and events, generating input signals.
  processParameterChanges(data.inputParameterChanges);
  processEvents(data.inputEvents);
  
  processSignals(data);
  return kResultTrue;
}

tresult PLUGIN_API PluginProcessor::setState(IBStream* state)
{
  // called when we load a preset, the model has to be reloaded
  IBStreamer streamer(state, kLittleEndian);
  
  int32 bypass;
  float cutoff, a, d, s, r;

  if(streamer.readInt32(bypass) == false) return kResultFalse;
  if(streamer.readFloat(cutoff) == false) return kResultFalse;
  if(streamer.readFloat(a) == false) return kResultFalse;
  if(streamer.readFloat(d) == false) return kResultFalse;
  if(streamer.readFloat(s) == false) return kResultFalse;
  if(streamer.readFloat(r) == false) return kResultFalse;

  bBypass = bypass > 0;
  fCutoff = cutoff;
  fAttack = a;
  fDecay = d;
  fSustain = s;
  fRelease = r;

  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::getState(IBStream* state)
{
  // here we need to save the model
  IBStreamer streamer(state, kLittleEndian);
  streamer.writeInt32(bBypass ? 1 : 0);
  streamer.writeFloat(fCutoff);
  streamer.writeFloat(fAttack);
  streamer.writeFloat(fDecay);
  streamer.writeFloat(fSustain);
  streamer.writeFloat(fRelease);
  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::setupProcessing(ProcessSetup& newSetup)
{
  // called before the process call, always in a disabled state(not active)
  _sampleRate = newSetup.sampleRate;
  
  // setup synth inputs
  _synthInput = std::make_unique< EventsToSignals >(_sampleRate);
  _synthInput->setPolyphony(kMaxVoices);
  
  // setup glides
  float glideTimeInSeconds{0.01f};
  _cutoffGlide.setGlideTimeInSamples(_sampleRate*glideTimeInSeconds);
  _cutoffGlide.setValue(0.5f);
  
  // reset voices
  for(auto& voice : _voices)
  {
    voice.clear();
  }
  
  // setup VST class
  return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API PluginProcessor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
  if(numIns)
  {
    if(SpeakerArr::getChannelCount(inputs[0]) != 0)
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

#pragma mark -
// private implementation

bool PluginProcessor::processParameterChanges(IParameterChanges* changes)
{
  if(changes)
  {
    int32 numParamsChanged = changes->getParameterCount();
    
    // for each parameter that changes in this audio block:
    for(int32 i = 0; i < numParamsChanged; ++i)
    {
      IParamValueQueue* paramQueue{nullptr};
      if(!(paramQueue = changes->getParameterData(i))) continue;
    
      ParamValue value;
      int sampleOffset;
      int numPoints = paramQueue->getPointCount();
      if(!(paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)) continue;

      int id = paramQueue->getParameterId();
    
      // std::cout << "param ID " << id << ", val " << value << " \n";
 
      if(id < (int)kNumPluginParameters)
      {
        // handle plugin parameters and convert from normalized values.
        // this is just a short example: a complete plugin framework would use a more
        // general Parameter object here.
        switch(id)
        {
          case kBypassId:
          {
            bBypass = (value > 0.5f);
            break;
          }
          case kCutoffId:
          {
            fCutoff = (float)value;
            break;
          }
          case kAttackId:
          {
            fAttack = (float)value*2.f;
            newEnvParams = true;
            break;
          }
          case kDecayId:
          {
            fDecay = (float)value*2.f;
            newEnvParams = true;
            break;
          }
          case kSustainId:
          {
            fSustain = (float)value;
            newEnvParams = true;
            break;
          }
          case kReleaseId:
          {
            fRelease = (float)value*8.f;
            newEnvParams = true;
            break;
          }
        }
      }
      else if(id < kVST3MIDITotalParams)
      {
        int midiID = id - kNumPluginParameters;
        int channel = midiID/kVST3MIDIParamsPerChannel;
        int paramIdx = midiID - channel*kVST3MIDIParamsPerChannel;
        
        //std::cout << "channel: " << channel << ", index: " << paramIdx << "\n";
        switch(paramIdx)
        {
          // special param: aftertouch
          case Steinberg::Vst::kAfterTouch:
          {
            _synthInput->addEvent({kNotePressure, channel, 0, sampleOffset,
              float(value), 0, 0, 0});
            break;
          }
          // special param: pitch bend
          case Steinberg::Vst::kPitchBend:
          {
            float bendValue = (value - 0.5f)*2.0f;
            _synthInput->addEvent({kPitchWheel, channel, 0, sampleOffset,
              bendValue, 0, 0, 0});
            break;
          }
          // special param: sustain pedal
          case Steinberg::Vst::kCtrlSustainOnOff:
          {
            _synthInput->addEvent({kSustainPedal, channel, 0, sampleOffset,
              float(value), 0, 0, 0});
            break;
          }
          // other params: send Controller # in event
          default:
          {
            _synthInput->addEvent({kController, channel, 0, sampleOffset,
              float(value), float(paramIdx), 0, 0});
            break;
          }
        }
      }
    }
  }
  return false; 
}

void PluginProcessor::processEvents (IEventList* events)
{
  if(!_synthInput.get()) return;
  
  // send all VST events to our EventsToSignals
  if (events)
  {
    int32 npos=0;
    int32 count = events->getEventCount ();
    for (int32 i = 0; i < count; i++)
    {
      Steinberg::Vst::Event e;
      events->getEvent(i, e);
      
      int channel{1};
      int creatorID{e.noteOn.pitch};
      int time{e.sampleOffset};
      float pitch = defaultScale.noteToLogPitch(e.noteOn.pitch);
      
      switch (e.type)
      {
        case Event::kNoteOnEvent:
        {
          _synthInput->addEvent({kNoteOn, channel, creatorID, time, pitch, e.noteOn.velocity});
          break;
        }
        case Event::kNoteOffEvent:
        {
          _synthInput->addEvent({kNoteOff, channel, creatorID, time, pitch, 0});
          break;
        }
        default:
          continue;
      }
    }
  }
}

// just an adapter so the VectorProcessBuffer utility can call our main process routine.
//
void PluginProcessorProcessVectorFn(MainInputs ins, MainOutputs outs, void* state)
{
  PluginProcessor* pProc = static_cast< PluginProcessor* >(state);
  return pProc->synthProcessVector(ins, outs);
} ;

// ProcessSignals() adapts the VST process() call with its arbitrary frame size to madronalib's
// fixed vector size processing.
//
void PluginProcessor::processSignals(ProcessData& data)
{
  if(data.numOutputs == 0)
  {
    // nothing to do
    return;
  }
  
  // our outputs are not silent
  data.outputs[0].silenceFlags = 0;
  
  // cast I/O pointers: necessary ugliness due to VST's use of void*
  void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
  float** outputs = reinterpret_cast<float**>(out);

  assert(processSetup.symbolicSampleSize == kSample32);
  
  // run buffered processing
  processBuffer.process(nullptr, outputs, data.numSamples, PluginProcessorProcessVectorFn, this);
}

// the main process routine! does everything.
//
void PluginProcessor::synthProcessVector(MainInputs inputs, MainOutputs outputs)
{
  // for console debugging when we need it
  bool debugFlag{false};
  _debugCounter += kFloatsPerDSPVector;
  if(_debugCounter > _sampleRate)
  {
    _debugCounter -= _sampleRate;
    //debugStuff();
    debugFlag = true;
  }
  
  // generate voice control signals from control events.
  if(_synthInput)
  {
    _synthInput->process();
  }
  
  if(newEnvParams)
  {
    for(auto& v : _voices)
    {
      v.setEnvParams(fAttack, fDecay, fSustain, fRelease, _sampleRate);
    }
    newEnvParams = false;
  }
  
  // clear outputs
  outputs[0] = DSPVector{0.f};
  outputs[1] = DSPVector{0.f};

  if(!bBypass)
  {
    // smooth float cutoff parameter to get cutoff signal
    auto c1 = _cutoffGlide(fCutoff);
    
    // process voices and sum to outputs
    for(int v=0; v < _synthInput->getPolyphony(); ++v)
    {
      auto& allocatorVoice = (_synthInput->voices)[v];
      auto& pitchSignal = allocatorVoice.outputs.row(kPitch);
      auto& gateSignal = allocatorVoice.outputs.row(kGate);
      
      // generate stereo voice output
      auto voiceOutput = _voices[v].processVector(pitchSignal, gateSignal, c1, _sampleRate, debugFlag);
      
      outputs[0] += voiceOutput.row(0);
      outputs[1] += voiceOutput.row(1);
    }
  }
}

// SynthVoice
#pragma mark -

void PluginProcessor::SynthVoice::setEnvParams(float a, float d, float s, float r, float sr)
{
  env1.coeffs = ADSR::calcCoeffs(a, d, s, r, sr);
}

// SynthVoice::processVector() is where an individual voice makes sound.
//
DSPVectorArray< 2 > PluginProcessor::SynthVoice::processVector(DSPVector pitch, DSPVector vel, DSPVector cutoff, float sr, bool debug)
{
  // convert 1/oct pitch to frequency
  constexpr float kFundamentalPitch = 440.f;
    
  // combine pitch with pitch bend
  constexpr float kBendSemitones = 7;
  constexpr float kBendRange = 1.0f*kBendSemitones/12;
  DSPVector fundamental(kFundamentalPitch);
  DSPVector freq = exp2Approx(pitch)*fundamental;
  DSPVector invSr(1.0f/sr);
  
  auto env = env1(vel);
  
  auto oscOut = osc1(freq*invSr);
  DSPVector k(0.5f); // constant res for now.
  
  // add fixed amount of env to cutoff
  DSPVector cutoffFreq = freq*cutoff*(DSPVector(1.0f) + DSPVector(8.0f)*env);
  
  // filter oscillator output
  auto filterOut = filt1(oscOut, cutoffFreq*invSr, k);
  
  // multiply by envelope and duplicate to get stereo output
  auto monoOut = filterOut*env;
  return concatRows(monoOut, monoOut);
}

void PluginProcessor::debugStuff()
{
  size_t p = _synthInput->getPolyphony();
  for(int v = 0; v < p; ++v )
  {
    auto& voice = (_synthInput->voices)[v];
    DSPVector vPitch = voice.outputs.row(kPitch);
    DSPVector vVel = voice.outputs.row(kGate);
    DSPVector vVoice = voice.outputs.row(kVoice);
    DSPVector vMod = voice.outputs.row(kMod);
    DSPVector vX = voice.outputs.row(kX);
    DSPVector vY = voice.outputs.row(kY);
    DSPVector vZ = voice.outputs.row(kZ);
    DSPVector vTime = voice.outputs.row(kElapsedTime);
    float vel = vVel[0];
    float pitch = vPitch[0];
    float vox = vVoice[0];
    float mod = vMod[0];
    float x = vX[0];
    float y = vY[0];
    float z = vZ[0];
    float time = vTime[0];
    
    if(0)
    {
      std::cout << "voice " << v << " : [" << vel << ", " << pitch << ", " << vox << ", " << mod << ", " << mod << "]\n";
      std::cout << "          [" << x << ", " << y << ", " << z << ", " << time << "]\n";
    }
  }
}
 
}}} // namespaces

