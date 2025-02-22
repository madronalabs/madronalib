//
// Created by Randy Jones on 2/21/25.
//

#include "MLAudioProcessor.h"
#include "rtaudio/RtAudio.h"

namespace ml
{

// all the info about the DSP task to be done
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
int RtAudioCallbackFn(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                             double /*streamTime*/, RtAudioStreamStatus status, void* callbackData)
{
  constexpr size_t kMaxIOChannels{64};

  // get process data from callback data
  auto pData = reinterpret_cast<RtAudioProcessData*>(callbackData);

  const float* pInputBuffer = reinterpret_cast<const float*>(inputBuffer);
  float* pOutputBuffer = reinterpret_cast<float*>(outputBuffer);

  if (status) std::cout << "Stream over/underflow detected." << std::endl;

  // make pointers to uninterlaced input and output frames for each channel.
  const float* inputs[kMaxIOChannels];
  float* outputs[kMaxIOChannels];

  // setup input and output pointers
  size_t nIns = std::min(kMaxIOChannels, pData->nInputs);
  size_t nOuts = std::min(kMaxIOChannels, pData->nOutputs);
  for (int i = 0; i < nIns; ++i)
  {
    inputs[i] = pInputBuffer + i * nBufferFrames;
  }
  for (int i = 0; i < nOuts; ++i)
  {
    outputs[i] = pOutputBuffer + i * nBufferFrames;
  }

  // do the buffered processing.
  pData->pProcessBuffer->process(inputs, outputs, nBufferFrames, pData->processFn,
                                 pData->processState);
  return 0;
}

struct AudioProcessor::Impl
{
  // the RtAudio controller
  RtAudio _adac;

  RtAudioProcessData processData;
};

// a free function that will be called when there is no function argument to a new AudioProcessor.
// In that case SignalProcessor::processVector will be called to do the processing.
void SignalProcessorProcessVectorFn(MainInputs ins, MainOutputs outs, void* state)
{
  SignalProcessor* pProc = static_cast<SignalProcessor*>(state);
  return pProc->processVector(ins, outs);
};

// the AudioProcessor constructor just fills in the data struct with everything needed to run
// the DSP graph. processFn points to a function that will be called by the VectorProcessBuffer.
// processState points to any persistent state that needs to be sent to the function. This can be
// unused if no state is needed, or if the state is global.
AudioProcessor::AudioProcessor(size_t nInputs, size_t nOutputs, int sampleRate,
                 ProcessVectorFn processFn, void* state)
    : pImpl(std::make_unique<Impl>()),
    SignalProcessor(nInputs, nOutputs)
{
  pImpl->processData.pProcessBuffer = &processBuffer;

  if (processFn)
  {
    pImpl->processData.processFn = processFn;
    pImpl->processData.processState = state;
  }
  else
  {
    pImpl->processData.processFn = SignalProcessorProcessVectorFn;
    pImpl->processData.processState = this;
  }

  pImpl->processData.nInputs = nInputs;
  pImpl->processData.nOutputs = nOutputs;
  pImpl->processData.sampleRate = sampleRate;
}

int AudioProcessor::startAudio()
{
  if (pImpl->_adac.getDeviceCount() < 1)
  {
    std::cout << "\nNo audio devices found!\n";
    return 0;
  }

  RtAudio::DeviceInfo info;
  uint32_t devices = pImpl->_adac.getDeviceCount();
  auto ids = pImpl->_adac.getDeviceIds();
  std::cout << "[AudioProcessor] Found: " << devices << " device(s)\n";
  for (uint32_t i = 0; i < devices; ++i)
  {
    info = pImpl->_adac.getDeviceInfo(ids[i]);
    std::cout << "\tDevice " << i << ": " << info.name << std::endl;
    std::cout << "\t\tinputs: " << info.inputChannels << " outputs: " << info.outputChannels << std::endl;
  }

  // Let RtAudio print messages to stderr.
  pImpl->_adac.showWarnings(true);

  // Set up RtAudio stream params
  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = pImpl->_adac.getDefaultInputDevice();
  iParams.nChannels = static_cast<unsigned int>(pImpl->processData.nInputs);
  iParams.firstChannel = 0;
  oParams.deviceId = pImpl->_adac.getDefaultOutputDevice();
  oParams.nChannels = static_cast<unsigned int>(pImpl->processData.nOutputs);
  oParams.firstChannel = 0;

  RtAudio::StreamOptions options;
  options.flags |= RTAUDIO_NONINTERLEAVED;

  auto pInputParams = (pImpl->processData.nInputs ? &iParams : nullptr);

  if (RTAUDIO_NO_ERROR !=
    pImpl->_adac.openStream(&oParams, pInputParams, RTAUDIO_FLOAT32, pImpl->processData.sampleRate,
                     &pImpl->processData.bufferFrames, &RtAudioCallbackFn, &pImpl->processData, &options))
  {
    std::cout << pImpl->_adac.getErrorText() << std::endl;
    return 0;
  }

  if (RTAUDIO_NO_ERROR !=
    pImpl->_adac.startStream())
  {
    std::cout << pImpl->_adac.getErrorText() << std::endl;
    return 0;
  }

  return 1;
}

void AudioProcessor::waitForEnterKey()
{
  char input;

  // Test RtAudio functionality for reporting latency.
  std::cout << "\nStream latency = " << pImpl->_adac.getStreamLatency() << " frames" << std::endl;
  std::cout << "sample rate: " << pImpl->processData.sampleRate << "\n";

  // wait for enter key.
  std::cout << "\nRunning ... press <enter> to quit (buffer frames = "
            << pImpl->processData.bufferFrames << ").\n";
  std::cin.get(input);
}

void AudioProcessor::stopAudio()
{
  if (RTAUDIO_NO_ERROR !=
      pImpl->_adac.stopStream())
  {
    std::cout << pImpl->_adac.getErrorText() << std::endl;
  }

  if (pImpl->_adac.isStreamOpen()) pImpl->_adac.closeStream();
}

int AudioProcessor::run()
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
void AudioProcessor::onMessage(Message msg)
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

AudioProcessor::~AudioProcessor() = default;

}