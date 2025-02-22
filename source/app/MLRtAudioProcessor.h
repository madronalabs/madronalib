// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// RtAudioProcessor: adaptor from RtAudio's main loop to madronalib vector processing

#pragma once
#include "MLSignalProcessor.h"
#include "rtaudio/RtAudio.h"
#include "mldsp.h"

namespace ml
{

// adapt the RtAudio process routine to a madronalib function operating on DSPBuffers.
int RtAudioCallbackFn(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                             double /*streamTime*/, RtAudioStreamStatus status, void* callbackData);

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


// a free function that will be called when there is no function argument to a new RtAudioProcessor.
// In that case SignalProcessor::processVector will be called to do the processing.
using processFnType = std::function<void(MainInputs, MainOutputs, void*)>;
inline void SignalProcessorProcessVectorFn(MainInputs ins, MainOutputs outs, void* state)
{
  SignalProcessor* pProc = static_cast<SignalProcessor*>(state);
  return pProc->processVector(ins, outs);
};

class RtAudioProcessor : public SignalProcessor, public Actor
{

  // all the info about the DSP task to be done
  RtAudioProcessData _processData;

  // the RtAudio controller
  RtAudio _adac;

public:
  RtAudioProcessor(size_t nInputs, size_t nOutputs, int sampleRate,
                   ProcessVectorFn processFn = nullptr, void* state = nullptr);

  ~RtAudioProcessor();

  inline int startAudio()
  {
    if (_adac.getDeviceCount() < 1)
    {
      std::cout << "\nNo audio devices found!\n";
      return 0;
    }

    RtAudio::DeviceInfo info;
    uint32_t devices = _adac.getDeviceCount();
    auto ids = _adac.getDeviceIds();
    std::cout << "[RtAudioProcessor] Found: " << devices << " device(s)\n";
    for (uint32_t i = 0; i < devices; ++i)
    {
      info = _adac.getDeviceInfo(ids[i]);
      std::cout << "\tDevice " << i << ": " << info.name << std::endl;
      std::cout << "\t\tinputs: " << info.inputChannels << " outputs: " << info.outputChannels << std::endl;
    }

    // Let RtAudio print messages to stderr.
    _adac.showWarnings(true);

    // Set up RtAudio stream params
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = _adac.getDefaultInputDevice();
    iParams.nChannels = static_cast<unsigned int>(_processData.nInputs);
    iParams.firstChannel = 0;
    oParams.deviceId = _adac.getDefaultOutputDevice();
    oParams.nChannels = static_cast<unsigned int>(_processData.nOutputs);
    oParams.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_NONINTERLEAVED;

    auto pInputParams = (_processData.nInputs ? &iParams : nullptr);

    if (RTAUDIO_NO_ERROR !=
      _adac.openStream(&oParams, pInputParams, RTAUDIO_FLOAT32, _processData.sampleRate,
                       &_processData.bufferFrames, &RtAudioCallbackFn, &_processData, &options))
    {
      std::cout << _adac.getErrorText() << std::endl;
      return 0;
    }

    if (RTAUDIO_NO_ERROR !=
      _adac.startStream())
    {
      std::cout << _adac.getErrorText() << std::endl;
      return 0;
    }

    return 1;
  }

  inline void waitForEnterKey()
  {
    char input;

    // Test RtAudio functionality for reporting latency.
    std::cout << "\nStream latency = " << _adac.getStreamLatency() << " frames" << std::endl;
    std::cout << "sample rate: " << _processData.sampleRate << "\n";

    // wait for enter key.
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = "
              << _processData.bufferFrames << ").\n";
    std::cin.get(input);
  }

  inline void stopAudio()
  {
    if (RTAUDIO_NO_ERROR !=
        _adac.stopStream())
    {
      std::cout << _adac.getErrorText() << std::endl;
    }
    
    if (_adac.isStreamOpen()) _adac.closeStream();
  }

  inline int run()
  {
    if (startAudio())
    {
      waitForEnterKey();
      stopAudio();
    }

#ifdef _WINDOWS
    system("pause");
#endif
    return 0;
  }

  // default Actor implementation
  inline void onMessage(Message msg) override
  {
    switch (hash(head(msg.address)))
    {
      case (hash("set_param")):
      {
        setParamFromNormalizedValue(tail(msg.address), msg.value.getFloatValue());
        break;
      }
      case (hash("set_prop")):
      {
        break;
      }
      case (hash("do")):
      {
        break;
      }
      default:
      {
        break;
      }
    }
  }

private:
  struct Impl;
  std::unique_ptr< Impl > pImpl;

};
} // namespace ml