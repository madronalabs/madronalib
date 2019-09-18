// example of RtAudio wrapping low-level madronalib DSP code.
// The reverb in this example is the Aaltoverb algorithm (madronalabs.com/products/Aaltoverb) without the tone control.

#include "mldsp.h"
#include "RtAudio.h"

using namespace ml;

constexpr int kProcessChannels = 2;
constexpr int kSampleRate = 44100;
constexpr int kMaxProcessBlockFrames = 4096;

// the VectorProcessBuffer buffers input from the RtAudio process routine and calls our process function.
VectorProcessBuffer<kProcessChannels, kMaxProcessBlockFrames> processBuffer;

// log projection for decay parameter
constexpr float kDecayLo = 0.8, kDecayHi = 20;
ml::Projection unityToDecay(ml::projections::intervalMap({0, 1}, {kDecayLo, kDecayHi}, ml::projections::log({kDecayLo, kDecayHi})));

// parameter smoothers
ml::LinearGlide mSmoothFeedback;
ml::LinearGlide mSmoothDelay;

// reverb machinery
ml::Hipass mHipassL, mHipassR;
ml::Allpass<ml::PitchbendableDelay> mAp1, mAp2, mAp3, mAp4;
ml::Allpass<ml::PitchbendableDelay> mAp5, mAp6, mAp7, mAp8, mAp9, mAp10;
ml::PitchbendableDelay mDelayL, mDelayR;
ml::OnePole mLp2, mLp3;

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

  // set initial delays to allocate delay memory
  mAp1.setDelayInSamples(500.f);
  mAp2.setDelayInSamples(500.f);
  mAp3.setDelayInSamples(1000.f);
  mAp4.setDelayInSamples(1000.f);
  mAp5.setDelayInSamples(2600.f);
  mAp6.setDelayInSamples(2600.f);
  mAp7.setDelayInSamples(8000.f);
  mAp8.setDelayInSamples(8000.f);
  mAp9.setDelayInSamples(10000.f);
  mAp10.setDelayInSamples(10000.f);
  mDelayL.setDelayInSamples(3500.f);
  mDelayR.setDelayInSamples(3500.f);

  // set fixed filter coefficients
  mLp2.mCoeffs = mLp3.mCoeffs = ml::OnePole::coeffs(20000.f/kSampleRate);
  mHipassL.mCoeffs = mHipassR.mCoeffs = ml::Hipass::coeffs(20.f/kSampleRate, 1.414f);
}


// do filter processing in DSPVector-sized chunks.
DSPVectorArray<2> processVectors(const DSPVectorArray<2>& inputVectors, int chans)
{
  const float sr = kSampleRate;
  const float RT60const = 0.001f;

  // size and decay parameters from 0-1.
  float sizeU = 0.5f;
  float decayU = 0.5f;

  // generate delay and feedback scalars
  float decayTime = unityToDecay(decayU);
  float decayIterations = decayTime/(sizeU);
  float feedback = (decayU < 1.0f) ? powf(RT60const, 1.0f/decayIterations) : 1.0f;

  // generate smoothed delay and feedback vectors
  DSPVector vSmoothDelay = mSmoothDelay(sizeU*2.0f);
  DSPVector vSmoothFeedback = mSmoothFeedback(feedback);

  // get smoothed allpass times
  DSPVector vt1 {0.00476*sr*vSmoothDelay};
  DSPVector vt2 {0.00358*sr*vSmoothDelay};
  DSPVector vt3 {0.00973*sr*vSmoothDelay};
  DSPVector vt4 {0.00830*sr*vSmoothDelay};
  DSPVector vt5 {0.029*sr*vSmoothDelay};
  DSPVector vt6 {0.021*sr*vSmoothDelay};
  DSPVector vt7 {0.078*sr*vSmoothDelay};
  DSPVector vt8 {0.090*sr*vSmoothDelay};
  DSPVector vt9 {0.111*sr*vSmoothDelay};
  DSPVector vt10 {0.096*sr*vSmoothDelay};

  // sum stereo inputs and diffuse with allpass filters
  DSPVector monoInput = (inputVectors.constRow(0) + inputVectors.constRow(1));
  DSPVector diffusedInput = mAp4(mAp3(mAp2(mAp1(monoInput, vt1), vt2), vt3), vt4);

  // get delay times in samples, subtracting the minimum delay of one DSPVector
  DSPVector vDelayTimeL = vSmoothDelay*DSPVector(0.0313*sr) - DSPVector(kFloatsPerDSPVector);
  DSPVector vDelayTimeR = vSmoothDelay*DSPVector(0.0371*sr) - DSPVector(kFloatsPerDSPVector);

  // sum diffused input with feedback, and apply late diffusion
  DSPVector vTapL = mHipassR(mAp7(mAp5(diffusedInput + mDelayL(mvFeedbackL, vDelayTimeL), vt5), vt7));
  DSPVector vTapR = mHipassL(mAp8(mAp6(diffusedInput + mDelayR(mvFeedbackR, vDelayTimeR), vt6), vt8));

  // remove lowpass filters when sustain = infinite
  if(feedback == 1.0f)
  {
    mLp2.mCoeffs = mLp3.mCoeffs = ml::OnePole::passthru();
  }
  else
  {
    mLp2.mCoeffs = mLp3.mCoeffs = ml::OnePole::coeffs(20000.f*(decayU*decayU)/sr);
  }

  // cross over late diffusion taps to feedback with lowpass filters
  mvFeedbackR = (mLp2(mAp9(vTapL, vt9))) * vSmoothFeedback;
  mvFeedbackL = (mLp3(mAp10(vTapR, vt10))) * vSmoothFeedback;

  auto stereoOutput = append(vTapL, vTapR);
  return stereoOutput;
}


/*
// do filter processing in DSPVector-sized chunks.
DSPVectorArray<2> processVectors(const DSPVectorArray<2>& inputVectors, int chans)
{
  // return inputVectors;

  return(append(s1(220.f/kSampleRate), s2(242.f/kSampleRate)));

}
*/

// adapt the RtAudio process routine to a madronalib function operating on DSPBuffers.
int callProcessVectorsBuffered( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
          double /*streamTime*/, RtAudioStreamStatus status, void *data )
{
  const float *pInputBuffer = reinterpret_cast<const float *>(inputBuffer);
  float *pOutputBuffer = reinterpret_cast<float *>(outputBuffer);

  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  // get pointers to uninterlaced input and output frames for each channel.
  const float* inputs[kProcessChannels]{};
  float* outputs[kProcessChannels]{};
  for(int i=0; i<kProcessChannels; ++i)
  {
    inputs[i] = pInputBuffer + i*nBufferFrames;
    outputs[i] = pOutputBuffer + i*nBufferFrames;
  }

  processBuffer.process(inputs, outputs, kProcessChannels, nBufferFrames,
                        [&](const DSPVectorArray<2> inputVectors, int chans)
                        {
                          return processVectors(inputVectors, chans);
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
  iParams.deviceId = adac.getDefaultInputDevice();
  iParams.nChannels = kProcessChannels;
  iParams.firstChannel = 0;
  oParams.deviceId = adac.getDefaultOutputDevice();
  oParams.nChannels = kProcessChannels;
  oParams.firstChannel = 0;

  RtAudio::StreamOptions options;
  options.flags |= RTAUDIO_NONINTERLEAVED;

  auto bufferBytes = bufferFrames*kProcessChannels*sizeof(float);

  initializeReverb();

  try
  {
    adac.openStream( &oParams, &iParams, RTAUDIO_FLOAT32, kSampleRate, &bufferFrames, &callProcessVectorsBuffered, (void *)&bufferBytes, &options );
  }
  catch ( RtAudioError& e )
  {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
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
  return 0;
}
