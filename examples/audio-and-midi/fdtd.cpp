// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// example of RtAudio wrapping low-level madronalib DSP code.

#include "MLAudioTask.h"

using namespace ml;

// context constants
constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kOutputGain = 0.1f;

// FDTD constants
constexpr int kWidth = 16;
constexpr int kHeight = 16;
constexpr int kPadding = 1;
constexpr int kRowStride = kWidth + kPadding*2;
constexpr int kTotalHeight = kHeight + kPadding*2;
const float kSize = sqrtf((kWidth*kWidth + 0.f) + (kHeight*kHeight + 0.f));
const float kInputGain = kWidth*kHeight/64;

using FDTDSurface = float[kRowStride*kTotalHeight];

struct FDTDState
{
  ImpulseGen impulse1;
  SineGen sine1;
  FDTDSurface u0{0};
  FDTDSurface u1{0};
  FDTDSurface u2{0};
  FDTDSurface* mpU0 = &u0;
  FDTDSurface* mpU1 = &u1;
  FDTDSurface* mpU2 = &u2;
};

void doFDTDStep2D(FDTDSurface* uIn1, FDTDSurface* uIn2, FDTDSurface* uOut, float kc, float ke, float kk, float kc2, float ke2)
{
  // get row pointers with padding
  float* pU = *uOut + kRowStride + kPadding;
  float* pU1 = *uIn1 + kRowStride + kPadding;
  float* pU2 = *uIn2 + kRowStride + kPadding;
  const float* pIn11, * pIn12, * pIn13, * pIn21, * pIn22, * pIn23;
  float* pOut;
  
  for(int j = 0; j < kHeight; ++j)
  {
    // U*z^-1 row pointers
    pIn11 = (pU1 + kRowStride*(j - 1));
    pIn12 = (pU1 + kRowStride*(j));
    pIn13 = (pU1 + kRowStride*(j + 1));
    // U*z^-2 row pointers
    pIn21 = (pU2 + kRowStride*(j - 1));
    pIn22 = (pU2 + kRowStride*(j));
    pIn23 = (pU2 + kRowStride*(j + 1));
    // output pointer
    pOut = (pU + kRowStride*(j));
    
    for(int i = 0; i < kWidth; i++)
    {
      float f = kc * pIn12[i]; // center z^-1
      f += ke * (pIn12[i-1] + pIn11[i] + pIn12[i+1] + pIn13[i]); // edges z^-1
      f += kk * (pIn11[i-1] + pIn11[i+1] + pIn13[i-1] + pIn13[i+1]); // corners z^-1
      f += kc2 * pIn22[i]; // center z^-2
      f += ke2 * (pIn22[i-1] + pIn21[i] + pIn22[i+1] + pIn23[i]); // edges z^-2
      pOut[i] = f;
    }
  }
}

// run the FDTD model with the given input and fundamental frequency.
// the frequency is updated every sample.

DSPVectorArray< 2 > processFDTDModel(DSPVector inputVec, DSPVector freq, FDTDState* state)
{
  const float isr = 1.0f/kSampleRate;
  DSPVector outLVec, outRVec;

  for(int i=0; i<kFloatsPerDSPVector; ++i)
  {
    // get frequency in cycles/sample
    float Fs = freq[i];
    
    // get approximate tension for fundamental freq. Fs
    float c = kSize*Fs;
    float T = 3.0f/5.0f*c;

    // set kernel values using
    // equal energy criterion: 4kk + 4ke + kc = 2.0.
    // the simulation is valid up to T^2 = 3/5, at which
    // the speed of wave travel is one mesh unit per time step.
    //
    // values outside the valid range WILL lead to blowups,
    // from which this demo makes no attempt to protect your
    // precious ears or speakers. please use caution.
    //
    float kk = T*T * (1.f / 6.f);
    float ke = T*T * (2.f / 3.f);
    float kc = 2.f - 4.f*(kk + ke);
    
    // get s0, freq. independent damping (approx range 1000 – 0)
    float s0 = 1.0f;
    
    // get s1, freq. dependent damping (approx range 1000 – 0)
    float s1 = 1.0f;

    // adjust kernel for freq. dependent damping constant
    float ks1 = s1*T*isr;
    ke += ks1;
    kc += -4.0f*ks1;
    float ke2 = -1.0f*ks1;
    float kc2 = s0*isr + 4.0f*ks1 - 1.0f;
    
    // premultiply entire kernel by independent damping constant SK
    float SK = 1.0f/(1.0f + isr*s0);
    kc *= SK;
    ke *= SK;
    kk *= SK;
    kc2 *= SK;
    ke2 *= SK;
    
    // excite surface with input at top center
    int exciteRow = 2;
    float* pU1 = *state->mpU1 + kRowStride + kPadding;
    float* exciter = pU1 + kRowStride*exciteRow + (kWidth/2);
    *exciter += inputVec[i]*kInputGain;
    
    // run the FDTD model for one sample
    // the model uses the state of the surface at the two previous time steps.
    doFDTDStep2D(state->mpU1, state->mpU2, state->mpU0, kc, ke, kk, kc2, ke2);
    
    // set float pickups at middle left and right
    int pickupRow = kHeight/2 + 1;
    float* pU0 = *state->mpU0 + kRowStride + kPadding;
    float* pickupL = pU0 + kRowStride*pickupRow + 1;
    float* pickupR = pU0 + kRowStride*pickupRow + kWidth - 1;

    // write sample from pickups to main outputs
    outLVec[i] = *pickupL;
    outRVec[i] = *pickupR;
    
    // finally, rotate buffer pointers
    auto temp = state->mpU2;
    state->mpU2 = state->mpU1;
    state->mpU1 = state->mpU0;
    state->mpU0 = temp;
  }
  
  // concatenating the two pickups makes a DSPVectorArray<2>: our stereo output.
  return concatRows(outLVec, outRVec);
}

// processFDTD() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.

void processFDTD(AudioContext* ctx, void *untypedState)
{
  auto state = static_cast<FDTDState*>(untypedState);

  // generate ticks twice per second
  auto ticks = state->impulse1(2.0f/kSampleRate)*kOutputGain;

  // run ticks through the FDTD model, modulating the pitch
  auto modOscSignal = state->sine1(0.15f/kSampleRate);
  auto freq = 220.f + modOscSignal*40.f;
  auto FDTDOutput = processFDTDModel(ticks, freq/kSampleRate, state);

  // write the main outputs
  ctx->outputs[0] = FDTDOutput.row(0);
  ctx->outputs[1] = FDTDOutput.row(1);
}

int main()
{
  FDTDState state;
  AudioContext ctx(kInputChannels, kOutputChannels, kSampleRate);
  AudioTask FDTDExample(&ctx, processFDTD, &state);
  return FDTDExample.run();
}
