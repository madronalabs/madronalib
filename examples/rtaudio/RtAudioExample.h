// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// RTAudioExample: plumbing to make examples using RtAudio more concise

#include "mldsp.h"
#include "RtAudio.h"

using namespace ml;

struct ProcessData
{
  VectorProcessBuffer* pProcessBuffer;
  ProcessVectorFn processFn;
  void* processState;
  size_t inputs;
  size_t outputs;
  int sampleRate;
};

// adapt the RtAudio process routine to a madronalib function operating on DSPBuffers.

int RtAudioCallbackFn( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                               double /*streamTime*/, RtAudioStreamStatus status , void * callbackData )
{
  // get process data from callback data
  auto pData = reinterpret_cast< ProcessData* >(callbackData);

  const float *pInputBuffer = reinterpret_cast<const float *>(inputBuffer);
  float *pOutputBuffer = reinterpret_cast<float *>(outputBuffer);

  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  // make pointers to uninterlaced input and output frames for each channel.
  const float* inputs[pData->inputs];
  float* outputs[pData->outputs];
  
  // setup input and output pointers
  for (int i = 0; i<pData->inputs; ++i)
  {
    inputs[i] = pInputBuffer + i*nBufferFrames;
  }
  for (int i = 0; i<pData->outputs; ++i)
  {
    outputs[i] = pOutputBuffer + i*nBufferFrames;
  }

  // do the buffered processing.
  pData->pProcessBuffer->process(inputs, outputs, nBufferFrames, pData->processFn, pData->processState);
  return 0;
}

int runRtAudioExample(ProcessData* pData)
{
    RtAudio adac;
    unsigned int bufferFrames = 512;

    if ( adac.getDeviceCount() < 1 )
    {
      std::cout << "\nNo audio devices found!\n";
      exit( 1 );
    }
    else
    {
      RtAudio::DeviceInfo info;
      uint32_t devices = adac.getDeviceCount();

      std::cout << "[rtaudio] Found: " << devices << " device(s)\n";

      for (uint32_t i = 0; i < devices; ++i)
      {
        info = adac.getDeviceInfo(i);
        std::cout << "\tDevice: " << i << " - " << info.name << std::endl;
      }
      std::cout << std::endl;
    }
    
    // Let RtAudio print messages to stderr.
    adac.showWarnings(true);

    // Set up RtAudio stream params
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = adac.getDefaultInputDevice();
    iParams.nChannels = pData->inputs;
    iParams.firstChannel = 0;
    oParams.deviceId = adac.getDefaultOutputDevice();
    oParams.nChannels = pData->outputs;
    oParams.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;
  
    auto pInputParams = (pData->inputs ? &iParams : nullptr);

    try
    {
      adac.openStream(&oParams, pInputParams, RTAUDIO_FLOAT32, pData->sampleRate, &bufferFrames, &RtAudioCallbackFn, pData, &options);
    }
    catch ( RtAudioError& e )
    {
      std::cout << '\n' << e.getMessage() << '\n' << std::endl;

  #ifdef _WINDOWS
      system("pause");
  #endif
      return 1;
    }

    // Test RtAudio functionality for reporting latency.
    std::cout << "\nStream latency = " << adac.getStreamLatency() << " frames" << std::endl;
    std::cout << "sample rate: " << pData->sampleRate << "\n";
  
    try
    {
      adac.startStream();
      char input;
      std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << bufferFrames << ").\n";
      std::cin.get(input);
      adac.stopStream();
    }
    catch ( RtAudioError& e )
    {
      std::cout << '\n' << e.getMessage() << '\n' << std::endl;
      goto cleanup;
    }

  cleanup:
    if ( adac.isStreamOpen() ) adac.closeStream();
  #ifdef _WINDOWS
    system("pause");
  #endif
    return 0;
}


class RtAudioExample
{
  // the maximum amount of input frames that can be proceesed at once. This determines the
  // maximum signal vector size of the plugin host or enclosing app.
  static constexpr int kMaxProcessBlockFrames {4096};
  
  // buffer object that splits up processing into DSPVectors
  VectorProcessBuffer processBuffer;
  
  // all the info about the DSP task to be done
  ProcessData _data;
  
public:
  
  // the RtAudioExample constructor just fills in the data struct with everything needed to run the DSP graph.
  // processState points to any persistent state that needs to be sent to the graph. This can be unused if
  // no state is needed, or if the state is global.
  
  RtAudioExample(size_t nInputs, size_t nOutputs, int sampleRate, ProcessVectorFn processFn, void* processState = nullptr) :
    processBuffer(nInputs, nOutputs, kMaxProcessBlockFrames)
  {
    _data.pProcessBuffer = &processBuffer;
    _data.processFn = processFn;
    _data.processState = processState;
    _data.inputs = nInputs;
    _data.outputs = nOutputs;
    _data.sampleRate = sampleRate;
  }
  
  int run()
  {
    return runRtAudioExample(&_data);
  }
};
