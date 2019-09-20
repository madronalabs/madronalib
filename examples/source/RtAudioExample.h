// plumbing to make examples using RtAudio more concise

#include "mldsp.h"
#include "RtAudio.h"

using namespace ml;

constexpr int kMaxProcessBlockFrames = 4096;

// adapt the RtAudio process routine to a madronalib function operating on DSPBuffers.
template<int IN_CHANS, int OUT_CHANS>
int callProcessVectorsBuffered( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                               double /*streamTime*/, RtAudioStreamStatus status , void *callbackData )
{
  // get std::function ptr from callback data
  using processFnType = std::function<DSPVectorArray<OUT_CHANS>(const DSPVectorArray<IN_CHANS>&)>;
  auto fp = reinterpret_cast<processFnType *>(callbackData);

  // the VectorProcessBuffer buffers input from the RtAudio process routine and calls our process function.
  static VectorProcessBuffer<IN_CHANS, OUT_CHANS, kMaxProcessBlockFrames> processBuffer;

  const float *pInputBuffer = reinterpret_cast<const float *>(inputBuffer);
  float *pOutputBuffer = reinterpret_cast<float *>(outputBuffer);

  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  // get pointers to uninterlaced input and output frames for each channel.
  const float* inputs[IN_CHANS]{};
  float* outputs[OUT_CHANS]{};
  for (int i = 0; i<IN_CHANS; ++i)
  {
    inputs[i] = pInputBuffer + i*nBufferFrames;
  }
  for (int i = 0; i<OUT_CHANS; ++i)
  {
    outputs[i] = pOutputBuffer + i*nBufferFrames;
  }

  // do the buffered processing.
  processBuffer.process(inputs, outputs, nBufferFrames, *fp);
  return 0;
}


int RunRtAudioExample(int exampleInputChannels, int exampleOutputChannels, int sampleRate, RtAudioCallback callbackFn, void* callbackData )
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
  adac.showWarnings( true );

  // Set up RtAudio stream params
  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = adac.getDefaultInputDevice();
  iParams.nChannels = exampleInputChannels;
  iParams.firstChannel = 0;
  oParams.deviceId = adac.getDefaultOutputDevice();
  oParams.nChannels = exampleOutputChannels;
  oParams.firstChannel = 0;

  RtAudio::StreamOptions options;
  options.flags |= RTAUDIO_NONINTERLEAVED;

  try
  {
    adac.openStream( &oParams, &iParams, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, callbackFn, callbackData, &options );
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
