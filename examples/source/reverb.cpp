// example of RtAudio wrapping low-level madronalib DSP code.
// The reverb in this example is the Aaltoverb algorithm (madronalabs.com/products/Aaltoverb) without the tone control.

#include "mldsp.h"
#include "RtAudio.h"

using namespace ml;

constexpr int kInputChannels = 2;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 44100;
constexpr int kMaxProcessBlockFrames = 4096;

// the VectorProcessBuffer buffers input from the RtAudio process routine and calls our process function.
VectorProcessBuffer<kInputChannels, kOutputChannels, kMaxProcessBlockFrames> processBuffer;

// log projection for decay parameter
constexpr float kDecayLo = 0.8, kDecayHi = 20;
Projection unityToDecay(projections::intervalMap({0, 1}, {kDecayLo, kDecayHi}, projections::log({kDecayLo, kDecayHi})));

// parameter smoothers
LinearGlide mSmoothFeedback;
LinearGlide mSmoothDelay;

// reverb machinery
Hipass mHipassL, mHipassR;
Allpass<PitchbendableDelay> mAp1, mAp2, mAp3, mAp4;
Allpass<PitchbendableDelay> mAp5, mAp6, mAp7, mAp8, mAp9, mAp10;
PitchbendableDelay mDelayL, mDelayR;
OnePole mLp2, mLp3;

// feedback storage
DSPVector mvFeedbackL, mvFeedbackR;

void initializeReverb()
{
  // set fixed parameters for reverb
  mSmoothFeedback.setGlideTimeInSamples(0.1f*kSampleRate);
  mSmoothDelay.setGlideTimeInSamples(0.1f*kSampleRate);

  // set allpass filter coefficients
  mAp1.mGain = 0.75f;
  mAp2.mGain = 0.70f;
  mAp3.mGain = 0.625f;
  mAp4.mGain = 0.625f;
  mAp5.mGain = mAp6.mGain = 0.7f;
  mAp7.mGain = mAp8.mGain = 0.6f;
  mAp9.mGain = mAp10.mGain = 0.5f;

  // allocate delay memory
  mAp1.setMaxDelayInSamples(500.f);
  mAp2.setMaxDelayInSamples(500.f);
  mAp3.setMaxDelayInSamples(1000.f);
  mAp4.setMaxDelayInSamples(1000.f);
  mAp5.setMaxDelayInSamples(2600.f);
  mAp6.setMaxDelayInSamples(2600.f);
  mAp7.setMaxDelayInSamples(8000.f);
  mAp8.setMaxDelayInSamples(8000.f);
  mAp9.setMaxDelayInSamples(10000.f);
  mAp10.setMaxDelayInSamples(10000.f);
  mDelayL.setMaxDelayInSamples(3500.f);
  mDelayR.setMaxDelayInSamples(3500.f);

  // set fixed filter coefficients
  mLp2.mCoeffs = mLp3.mCoeffs = OnePole::coeffs(20000.f/kSampleRate);
  mHipassL.mCoeffs = mHipassR.mCoeffs = Hipass::coeffs(20.f/kSampleRate, 1.414f);
}

// do filter processing in DSPVector-sized chunks.
DSPVectorArray<2> processVectors(const DSPVectorArray<2>& inputVectors)
{
  const float sr = kSampleRate;
  const float RT60const = 0.001f;

  // size and decay parameters from 0-1. It will be more interesting to change these over time in some way.
  float sizeU = 0.5f;
  float decayU = 0.5f;

  // generate delay and feedback scalars
  float decayTime = unityToDecay(decayU);
  float decayIterations = decayTime/(sizeU*0.5);
  float feedback = (decayU < 1.0f) ? powf(RT60const, 1.0f/decayIterations) : 1.0f;

  // generate smoothed delay and feedback vectors
  DSPVector vSmoothDelay = mSmoothDelay(sizeU*2.0f);
  DSPVector vSmoothFeedback = mSmoothFeedback(feedback);

  // get smoothed allpass times
  DSPVector vMin(kFloatsPerDSPVector);
  DSPVector vt1 = max(DSPVector(0.00476*sr*vSmoothDelay), vMin);
  DSPVector vt2 = max(DSPVector(0.00358*sr*vSmoothDelay), vMin);
  DSPVector vt3 = max(DSPVector(0.00973*sr*vSmoothDelay), vMin);
  DSPVector vt4 = max(DSPVector(0.00830*sr*vSmoothDelay), vMin);
  DSPVector vt5 = max(DSPVector(0.029*sr*vSmoothDelay), vMin);
  DSPVector vt6 = max(DSPVector(0.021*sr*vSmoothDelay), vMin);
  DSPVector vt7 = max(DSPVector(0.078*sr*vSmoothDelay), vMin);
  DSPVector vt8 = max(DSPVector(0.090*sr*vSmoothDelay), vMin);
  DSPVector vt9 = max(DSPVector(0.111*sr*vSmoothDelay), vMin);
  DSPVector vt10 = max(DSPVector(0.096*sr*vSmoothDelay), vMin);


  // sum stereo inputs and diffuse with allpass filters
  DSPVector monoInput = (inputVectors.constRow(0) + inputVectors.constRow(1));
  DSPVector diffusedInput = mAp4(mAp3(mAp2(mAp1(monoInput, vt1), vt2), vt3), vt4);

  // get delay times in samples, subtracting the constant delay of one DSPVector and clamping
  DSPVector vDelayTimeL = max(vSmoothDelay*DSPVector(0.0313*sr) - vMin, DSPVector(0.f));
  DSPVector vDelayTimeR = max(vSmoothDelay*DSPVector(0.0371*sr) - vMin, DSPVector(0.f));

  // sum diffused input with feedback, and apply late diffusion
  DSPVector vTapL = mHipassR(mAp7(mAp5(diffusedInput + mDelayL(mvFeedbackL, vDelayTimeL), vt5), vt7));
  DSPVector vTapR = mHipassL(mAp8(mAp6(diffusedInput + mDelayR(mvFeedbackR, vDelayTimeR), vt6), vt8));

  // get lowpass filter coeffs (passthru when decay = infinite)
  if(feedback == 1.0f)
  {
    mLp2.mCoeffs = mLp3.mCoeffs = OnePole::passthru();
  }
  else
  {
    mLp2.mCoeffs = mLp3.mCoeffs = OnePole::coeffs(20000.f*(decayU*decayU)/sr);
  }

  // cross over late diffusion taps to feedback with lowpass filters
  mvFeedbackR = (mLp2(mAp9(vTapL, vt9))) * vSmoothFeedback;
  mvFeedbackL = (mLp3(mAp10(vTapR, vt10))) * vSmoothFeedback;

  auto stereoOutput = append(vTapL, vTapR);
  return stereoOutput;
}

// adapt the RtAudio process routine to a madronalib function operating on DSPBuffers.
int callProcessVectorsBuffered( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
          double /*streamTime*/, RtAudioStreamStatus status , void *data )
{
  const float *pInputBuffer = reinterpret_cast<const float *>(inputBuffer);
  float *pOutputBuffer = reinterpret_cast<float *>(outputBuffer);

  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  // get pointers to uninterlaced input and output frames for each channel.
  const float* inputs[kInputChannels]{};
  float* outputs[kOutputChannels]{};
  for (int i = 0; i<kInputChannels; ++i)
  {
    inputs[i] = pInputBuffer + i*nBufferFrames;
  }
  for (int i = 0; i<kOutputChannels; ++i)
  {
    outputs[i] = pOutputBuffer + i*nBufferFrames;
  }

  processBuffer.process(inputs, outputs, nBufferFrames,
                        [&](const DSPVectorArray<2> inputVectors)
                        {
                          return processVectors(inputVectors);
                        } );
  return 0;
}

int main( int argc, char *argv[] )
{
  RtAudio adac;

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

  // Set the same number of channels for both input and output.
  unsigned int bufferFrames = 512;
  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = 0;// adac.getDefaultInputDevice();
  iParams.nChannels = 0;// kInputChannels;
  iParams.firstChannel = 0;
  oParams.deviceId = adac.getDefaultOutputDevice();
  oParams.nChannels = kOutputChannels;
  oParams.firstChannel = 0;

  RtAudio::StreamOptions options;
  options.flags |= RTAUDIO_NONINTERLEAVED;


  initializeReverb();

  try
  {
    adac.openStream( &oParams, /*&iParams*/ 0, RTAUDIO_FLOAT32, kSampleRate, &bufferFrames, &callProcessVectorsBuffered, nullptr, &options );
  }
  catch ( RtAudioError& e )
  {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;

#ifdef _WINDOWS
    system("pause");
#endif
    exit( 1 );
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
