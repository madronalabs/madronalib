//
// Created by Randy Jones on 2/21/25.
//

#include "MLPlatform.h"
#include "MLAudioContext.h"
#include "MLAudioTask.h"
#include "MLSignalProcessBuffer.h"

#include "rtaudio/RtAudio.h"

namespace ml
{

#ifdef ML_MAC
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

void waitForConsoleKeyPress()
{
  char ch{EOF};
  struct termios oldt, newt;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  while (ch == EOF)
  {
    ch = getchar();
    std::this_thread::sleep_for(10ms);
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
}

#endif  // ML_MAC
#ifdef ML_WINDOWS
#include <conio.h>

char keyPressedAsync()
{
  for (int i = 0x07; i < 256; ++i)
  {
    if (GetAsyncKeyState(i) & 0x8000)
    {
      return i;
    }
  }
  return false;
}

void waitForConsoleKeyPress()
{
  // Hide the console cursor
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_CURSOR_INFO cursorInfo;
  GetConsoleCursorInfo(hConsole, &cursorInfo);
  cursorInfo.bVisible = false;  // Set the cursor visibility
  SetConsoleCursorInfo(hConsole, &cursorInfo);

  // Wait for a key press
  while (true)
  {
    if (_kbhit())
    {
      int ch = _getch();  // Read the key press
      break;
    }
    std::this_thread::sleep_for(10ms);
  }

  // Restore the console cursor
  cursorInfo.bVisible = true;
  SetConsoleCursorInfo(hConsole, &cursorInfo);
}

#endif  // ML_WINDOWS

#if !defined(ML_MAC) && !defined(ML_WINDOWS)
// Fallback implementation for other platforms (Linux, etc.)
void waitForConsoleKeyPress()
{
  std::cout << "Press Enter to continue...";
  std::cin.get();
}
#endif

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

  // Buffer the data to and from the outside world and run the process in DSPVector-sized chunks
  // within the context.
  pData->buffer->process(inputs, outputs, nBufferFrames, pData->processContext, pData->processFn,
                         pData->processState);
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
    std::cout << "\t\tinputs: " << info.inputChannels << " outputs: " << info.outputChannels
              << std::endl;
  }

  // Let RtAudio print messages to stderr.
  pImpl->adac.showWarnings(true);

  auto nInputs = pImpl->processData.processContext->inputs.size();
  auto nOutputs = pImpl->processData.processContext->outputs.size();
  int sampleRate = pImpl->processData.processContext->getSampleRate();
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

  if (RTAUDIO_NO_ERROR != pImpl->adac.openStream(&oParams, pInputParams, RTAUDIO_FLOAT32,
                                                 sampleRate, &bufferFrames, &RtAudioCallbackFn,
                                                 &pImpl->processData, &options))
  {
    std::cout << pImpl->adac.getErrorText() << std::endl;
    return 0;
  }

  if (RTAUDIO_NO_ERROR != pImpl->adac.startStream())
  {
    std::cout << pImpl->adac.getErrorText() << std::endl;
    return 0;
  }

  return 1;
}

void AudioTask::stopAudio()
{
  if (RTAUDIO_NO_ERROR != pImpl->adac.stopStream())
  {
    std::cout << pImpl->adac.getErrorText() << std::endl;
  }

  if (pImpl->adac.isStreamOpen()) pImpl->adac.closeStream();
}

int AudioTask::runConsoleApp()
{
  if (startAudio())
  {
    // Test RtAudio functionality for reporting latency.
    std::cout << "\nStream latency = " << pImpl->adac.getStreamLatency() << " frames" << std::endl;
    std::cout << "sample rate: " << pImpl->processData.processContext->getSampleRate() << "\n";

    // wait for enter key.
    std::cout << "\nRunning ... press any key to quit.\n";
    waitForConsoleKeyPress();

    stopAudio();
  }

  return 0;
}

AudioTask::~AudioTask() = default;

}  // namespace ml
