//
// Created by Randy Jones on 2/21/25.
//

#include "MLAudioContext.h"
#include "MLAudioTask.h"
#include "MLSignalProcessBuffer.h"

#include "rtaudio/RtAudio.h"

namespace ml
{

constexpr int kRtAudioCallbackFrames{512};

struct AudioProcessData
{
  // buffered processing
  std::unique_ptr<SignalProcessBuffer> buffer;

  // context, function and state for the process.
  AudioContext* processContext{nullptr};
  SignalProcessFn processFn{nullptr};
  void* processState{nullptr};
};

struct AudioTask::Impl
{
  // native audio task
  RtAudio adac;

  AudioProcessData processData;

  Impl(size_t nInputs, size_t nOutputs, size_t kMaxBlockSize)
  {
    processData.buffer = std::make_unique<SignalProcessBuffer>(nInputs, nOutputs, kMaxBlockSize);
  }
};

// adapt the RtAudio process routine to a madronalib function operating on DSPBuffers.
int RtAudioCallbackFn(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                             double /*streamTime*/, RtAudioStreamStatus status, void* callbackData)
{
  constexpr size_t kMaxIOChannels{64};

  // get process data from callback data
  auto pData = reinterpret_cast<AudioProcessData*>(callbackData);

  if (status) std::cout << "Stream over/underflow detected." << std::endl;

  // make pointers to uninterlaced input and output frames for each channel.
  const float* inputs[kMaxIOChannels];
  float* outputs[kMaxIOChannels];

  // setup input and output pointers
  const float* pInputBuffer = reinterpret_cast<const float*>(inputBuffer);
  float* pOutputBuffer = reinterpret_cast<float*>(outputBuffer);
  size_t nIns = std::min(kMaxIOChannels, pData->processContext->inputs.size());
  size_t nOuts = std::min(kMaxIOChannels, pData->processContext->outputs.size());
  for (int i = 0; i < nIns; ++i)
  {
    inputs[i] = pInputBuffer + i * nBufferFrames;
  }
  for (int i = 0; i < nOuts; ++i)
  {
    outputs[i] = pOutputBuffer + i * nBufferFrames;
  }

  // Buffer the data to and from the outside world and run the process in DSPVector-sized chunks within the context.
  pData->buffer->process(inputs, outputs, nBufferFrames,
    pData->processContext, pData->processFn, pData->processState);
  return 0;
}


// the AudioTask constructor fills in the processData struct with everything needed to run audio.
// the DSP function. processFn points to a function that will be called by the SignalProcessBuffer.
// state points to any persistent state that needs to be sent to the function.

AudioTask::AudioTask(AudioContext* ctx, SignalProcessFn processFn, void* state)
{
  // make the world -> context buffers for each channel
  pImpl = std::make_unique<Impl>(ctx->inputs.size(), ctx->outputs.size(), kMaxBlockSize);

  pImpl->processData.processContext = ctx;
  pImpl->processData.processFn = processFn;
  pImpl->processData.processState = state;
}


int AudioTask::startAudio()
{
  if (pImpl->adac.getDeviceCount() < 1)
  {
    std::cout << "\nNo audio devices found!\n";
    return 0;
  }

  RtAudio::DeviceInfo info;
  uint32_t devices = pImpl->adac.getDeviceCount();
  auto ids = pImpl->adac.getDeviceIds();
  std::cout << "[AudioTask] Found: " << devices << " device(s)\n";
  for (uint32_t i = 0; i < devices; ++i)
  {
    info = pImpl->adac.getDeviceInfo(ids[i]);
    std::cout << "\tDevice " << i << ": " << info.name << std::endl;
    std::cout << "\t\tinputs: " << info.inputChannels << " outputs: " << info.outputChannels << std::endl;
  }

  // Let RtAudio print messages to stderr.
  pImpl->adac.showWarnings(true);

  int nInputs = pImpl->processData.processContext->inputs.size();
  int nOutputs = pImpl->processData.processContext->outputs.size();
  int sampleRate = pImpl->processData.processContext->sampleRate;
  unsigned int bufferFrames = kRtAudioCallbackFrames;

  // Set up RtAudio stream params
  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = pImpl->adac.getDefaultInputDevice();
  iParams.nChannels = static_cast<unsigned int>(nInputs);
  iParams.firstChannel = 0;
  oParams.deviceId = pImpl->adac.getDefaultOutputDevice();
  oParams.nChannels = static_cast<unsigned int>(nOutputs);
  oParams.firstChannel = 0;

  RtAudio::StreamOptions options;
  options.flags |= RTAUDIO_NONINTERLEAVED;

  auto pInputParams = (nInputs ? &iParams : nullptr);

  if (RTAUDIO_NO_ERROR !=
    pImpl->adac.openStream(&oParams, pInputParams, RTAUDIO_FLOAT32, sampleRate,
                     &bufferFrames, &RtAudioCallbackFn, &pImpl->processData, &options))
  {
    std::cout << pImpl->adac.getErrorText() << std::endl;
    return 0;
  }

  if (RTAUDIO_NO_ERROR !=
    pImpl->adac.startStream())
  {
    std::cout << pImpl->adac.getErrorText() << std::endl;
    return 0;
  }

  return 1;
}


void AudioTask::waitForEnterKey()
{
  char input;

  // Test RtAudio functionality for reporting latency.
  std::cout << "\nStream latency = " << pImpl->adac.getStreamLatency() << " frames" << std::endl;
  std::cout << "sample rate: " << pImpl->processData.processContext->sampleRate << "\n";

  // wait for enter key.
  std::cout << "\nRunning ... press <enter> to quit.\n";
  std::cin.get(input);
}

void AudioTask::stopAudio()
{
  if (RTAUDIO_NO_ERROR !=
      pImpl->adac.stopStream())
  {
    std::cout << pImpl->adac.getErrorText() << std::endl;
  }

  if (pImpl->adac.isStreamOpen()) pImpl->adac.closeStream();
}

int AudioTask::run()
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

AudioTask::~AudioTask() = default;

}
