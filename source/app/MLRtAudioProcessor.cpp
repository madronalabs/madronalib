//
// Created by Randy Jones on 2/21/25.
//

#include "MLRtAudioProcessor.h"

namespace ml
{

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



struct RtAudioProcessor::Impl
{
int test;
};



// the RtAudioProcessor constructor just fills in the data struct with everything needed to run
// the DSP graph. processFn points to a function that will be called by the VectorProcessBuffer.
// processState points to any persistent state that needs to be sent to the function. This can be
// unused if no state is needed, or if the state is global.
RtAudioProcessor::RtAudioProcessor(size_t nInputs, size_t nOutputs, int sampleRate,
                 ProcessVectorFn processFn, void* state)
    : pImpl(std::make_unique<Impl>()),
    SignalProcessor(nInputs, nOutputs)
{
  _processData.pProcessBuffer = &processBuffer;

  if (processFn)
  {
    _processData.processFn = processFn;
    _processData.processState = state;
  }
  else
  {
    _processData.processFn = SignalProcessorProcessVectorFn;
    _processData.processState = this;
  }

  _processData.nInputs = nInputs;
  _processData.nOutputs = nOutputs;
  _processData.sampleRate = sampleRate;
}

RtAudioProcessor::~RtAudioProcessor() = default;

}