// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// RtAudioProcessor: adaptor from RtAudio's main loop to madronalib vector processing

#pragma once
#include "mldsp.h"
#include "RtAudio.h"
#include "MLSignalProcessor.h"

using namespace ml;

struct RtAudioProcessData
{
  VectorProcessBuffer* pProcessBuffer;
  ProcessVectorFn processFn;
  void* processState;
  size_t nInputs;
  size_t nOutputs;
  int sampleRate;
  unsigned int bufferFrames{512};
};

// adapt the RtAudio process routine to a madronalib function operating on DSPBuffers.

inline int RtAudioCallbackFn( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                               double /*streamTime*/, RtAudioStreamStatus status , void * callbackData )
{
  constexpr size_t kMaxIOChannels{64};
  
  // get process data from callback data
  auto pData = reinterpret_cast< RtAudioProcessData* >(callbackData);

  const float *pInputBuffer = reinterpret_cast<const float *>(inputBuffer);
  float *pOutputBuffer = reinterpret_cast<float *>(outputBuffer);

  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  // make pointers to uninterlaced input and output frames for each channel.
  const float* inputs[kMaxIOChannels];
  float* outputs[kMaxIOChannels];
  
  // setup input and output pointers
  size_t nIns = std::min(kMaxIOChannels, pData->nInputs);
  size_t nOuts = std::min(kMaxIOChannels, pData->nOutputs);
  for (int i = 0; i<nIns; ++i)
  {
    inputs[i] = pInputBuffer + i*nBufferFrames;
  }
  for (int i = 0; i<nOuts; ++i)
  {
    outputs[i] = pOutputBuffer + i*nBufferFrames;
  }

  // do the buffered processing.
  pData->pProcessBuffer->process(inputs, outputs, nBufferFrames, pData->processFn, pData->processState);
  return 0;
}


// a free function that will be called when there is no function argument to a new RtAudioProcessor.
// In that case SignalProcessor::processVector will be called to do the processing.
using processFnType = std::function< void(MainInputs, MainOutputs, void *) >;
inline void SignalProcessorProcessVectorFn(MainInputs ins, MainOutputs outs, void* state)
{
  SignalProcessor* pProc = static_cast< SignalProcessor* >(state);
  return pProc->processVector(ins, outs);
} ;


class RtAudioProcessor :
  public SignalProcessorActor
{
  // all the info about the DSP task to be done
  RtAudioProcessData _data;
  
  // the RtAudio controller
  RtAudio _adac;
  
public:
  
  // the RtAudioProcessor constructor just fills in the data struct with everything needed to run the DSP graph.
  // processFn points to a function that will be called by the VectorProcessBuffer.
  // processState points to any persistent state that needs to be sent to the function. This can be unused if
  // no state is needed, or if the state is global.
  RtAudioProcessor(size_t nInputs, size_t nOutputs, int sampleRate, ProcessVectorFn processFn = nullptr, void* state = nullptr) :
    SignalProcessorActor(nInputs, nOutputs)
  {
    _data.pProcessBuffer = &processBuffer;
    
    if(processFn)
    {
      _data.processFn = processFn;
      _data.processState = state;
    }
    else
    {
      _data.processFn = SignalProcessorProcessVectorFn;
      _data.processState = this;
    }
    
    _data.nInputs = nInputs;
    _data.nOutputs = nOutputs;
    _data.sampleRate = sampleRate;
  }
  
  ~RtAudioProcessor() = default;
  
  inline int startAudio()
  {
    if ( _adac.getDeviceCount() < 1 )
    {
      std::cout << "\nNo audio devices found!\n";
      return 0;
    }
    else
    {
      RtAudio::DeviceInfo info;
      uint32_t devices = _adac.getDeviceCount();
      
      std::cout << "[rtaudio] Found: " << devices << " device(s)\n";
      
      for (uint32_t i = 0; i < devices; ++i)
      {
        info = _adac.getDeviceInfo(i);
        std::cout << "\tDevice: " << i << " - " << info.name << std::endl;
      }
      std::cout << std::endl;
    }
    
    // Let RtAudio print messages to stderr.
    _adac.showWarnings(true);
    
    // Set up RtAudio stream params
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = _adac.getDefaultInputDevice();
    iParams.nChannels = _data.nInputs;
    iParams.firstChannel = 0;
    oParams.deviceId = _adac.getDefaultOutputDevice();
    oParams.nChannels = _data.nOutputs;
    oParams.firstChannel = 0;
    
    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;
    
    auto pInputParams = (_data.nInputs ? &iParams : nullptr);
    
    try
    {
      _adac.openStream(&oParams, pInputParams, RTAUDIO_FLOAT32, _data.sampleRate, &_data.bufferFrames, &RtAudioCallbackFn, &_data, &options);
    }
    catch ( RtAudioError& e )
    {
      std::cout << '\n' << e.getMessage() << '\n' << std::endl;
      return 0;
    }
    
    try
    {
      _adac.startStream();
    }
    catch ( RtAudioError& e )
    {
      std::cout << '\n' << e.getMessage() << '\n' << std::endl;
      return 0;
    }

    return 1;
  }
  
  inline void waitForEnterKey()
  {
    char input;
    
    // Test RtAudio functionality for reporting latency.
    std::cout << "\nStream latency = " << _adac.getStreamLatency() << " frames" << std::endl;
    std::cout << "sample rate: " << _data.sampleRate << "\n";
    
    // wait for enter key.
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << _data.bufferFrames << ").\n";
    std::cin.get(input);
  }
  
  inline void stopAudio()
  {
    try
    {
      _adac.stopStream();
    }
    catch ( RtAudioError& e )
    {
      std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    }
    
    if ( _adac.isStreamOpen() ) _adac.closeStream();
  }
  
  inline int run()
  {
    if(startAudio())
    {
      waitForEnterKey();
      stopAudio();
    }
    
#ifdef _WINDOWS
    system("pause");
#endif
    return 0;
  }
};
