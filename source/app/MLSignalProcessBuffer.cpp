//
// Created by Randy Jones on 2/27/25.
//

#include "MLDSPBuffer.h"
#include "MLDSPOps.h"
#include "MLSignalProcessBuffer.h"

using namespace ml;
namespace ml
{
// SignalProcessBuffer: utility class to serve a main loop with varying
// arbitrary chunk sizes, buffer inputs and outputs, and compute DSP in
// DSPVector-sized chunks.

SignalProcessBuffer::SignalProcessBuffer(size_t inputs, size_t outputs, size_t maxFrames)
    : maxFrames_(maxFrames)
{
  inputBuffers_.resize(inputs);
  for (int i = 0; i < inputs; ++i)
  {
    inputBuffers_[i].resize((int)maxFrames_);
  }

  outputBuffers_.resize(outputs);
  for (int i = 0; i < outputs; ++i)
  {
    outputBuffers_[i].resize((int)maxFrames_);
  }
}

SignalProcessBuffer::~SignalProcessBuffer() {}

// Buffer the external context and provide an internal context for the process function.
// Then run the process function in the internal context, updating its state.
void SignalProcessBuffer::process(const float** externalInputs, float** externalOutputs,
                                  int externalFrames, AudioContext* context,
                                  SignalProcessFn processFn, void* state)
{
  size_t nInputs = inputBuffers_.size();
  size_t nOutputs = outputBuffers_.size();
  if (nOutputs < 1) return;
  if (!externalOutputs) return;
  if (externalFrames > (int)maxFrames_) return;

  // write vectors from external inputs (if any) to inputBuffers
  for (int c = 0; c < nInputs; c++)
  {
    if (externalInputs[c])
    {
      inputBuffers_[c].write(externalInputs[c], externalFrames);
    }
  }

  // run vector-size process until we have externalFrames of output
  int startOffset{0};
  while (outputBuffers_[0].getReadAvailable() < externalFrames)
  {
    // read one chunk from each input buffer
    for (int c = 0; c < nInputs; c++)
    {
      // read one DSPVector from the input buffer.
      context->inputs[c] = inputBuffers_[c].read();
    }

    // process one vector of the context, generating event / controller signals
    context->processVector(startOffset);
    startOffset += kFloatsPerDSPVector;

    // run the signal processing function
    processFn(context, state);

    // write one vector to each output buffer
    for (int c = 0; c < nOutputs; c++)
    {
      outputBuffers_[c].write(context->outputs[c]);
    }
  }

  // read from outputBuffers to external outputs
  for (int c = 0; c < nOutputs; c++)
  {
    if (externalOutputs[c])
    {
      outputBuffers_[c].read(externalOutputs[c], externalFrames);
    }
  }

  context->clearInputEvents();
}

}  // namespace ml
