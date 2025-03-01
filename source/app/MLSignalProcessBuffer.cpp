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
  : _maxFrames(maxFrames)
{
  _inputBuffers.resize(inputs);
  for (int i = 0; i < inputs; ++i)
  {
    _inputBuffers[i].resize((int)_maxFrames);
  }

  _outputBuffers.resize(outputs);
  for (int i = 0; i < outputs; ++i)
  {
    _outputBuffers[i].resize((int)_maxFrames);
  }
}

SignalProcessBuffer::~SignalProcessBuffer() {}

// Buffer the external context and provide an internal context for the process function.
// Then run the process function in the internal context, updating its state.
void SignalProcessBuffer::process(const float** externalInputs, float** externalOutputs, int nFrames,
  AudioContext* context, SignalProcessFn processFn, void* state)
{
  size_t nInputs = _inputBuffers.size();
  size_t nOutputs = _outputBuffers.size();
  if (nOutputs < 1) return;
  if(!externalOutputs) return;
  if (nFrames > (int)_maxFrames) return;

  // write vectors from inputs (if any) to inputBuffers
  for(int c = 0; c < nInputs; c++)
  {
    if(externalInputs[c])
    {
      _inputBuffers[c].write(externalInputs[c], nFrames);
    }
  }

  // run vector-size process until we have nFrames of output
  while(_outputBuffers[0].getReadAvailable() < nFrames)
  {
    // read one chunk from each input buffer
    for(int c = 0; c < nInputs; c++)
    {
      context->inputs[c] = _inputBuffers[c].read();
    }

    context->processVector();
    processFn(context, state);

    // write one chunk to each output buffer
    for(int c = 0; c < nOutputs; c++)
    {
      _outputBuffers[c].write(context->outputs[c]);
    }
  }

  // read from outputBuffers to outputs
  for(int c = 0; c < nOutputs; c++)
  {
    if(externalOutputs[c])
    {
      _outputBuffers[c].read(externalOutputs[c], nFrames);
    }
  }
}

} // ml