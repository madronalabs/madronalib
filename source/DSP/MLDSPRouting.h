// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#ifdef _WIN32
#include <memory>
#else
// #include <tr1/memory>
#endif

#ifdef __INTEL_COMPILER
#include <mathimf.h>
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <type_traits>

#include "MLDSPMath.h"
#include "MLDSPScalarMath.h"

namespace ml
{
// ----------------------------------------------------------------
// ways to combine vectors: mixers, mults, panners and gates for example.
// if no state is required these will be functions, otherwise function objects.
/*
Mixer
Switcher
 ProcessFader - SmoothSwitcher -
Panner
Gate
Switch
Comparator
Parallel
 Bank


*/

// mix (DSPVectorArray<INPUTS>gains, a, b, c, ... )
// returns the sum of each input DSPVectorArray multiplied by the corresponding row
// of the gains array.
// NOTE: if the gains DSPVectorArray contains a number of rows smaller than the number of inputs,
// unsafe behavior results.
// TODO: with the right template-fu it should be possible to check the sizes at compile time,
// and to avoid passing inputIndex at runtime.

template <size_t ROWS, size_t INPUTS, typename... Args>
DSPVectorArray<ROWS> mix_n(size_t inputIndex, DSPVectorArray<INPUTS> gains,
                           DSPVectorArray<ROWS> first)
{
  return first * repeatRows<ROWS>(gains.getRowVectorUnchecked(inputIndex));
}

template <size_t ROWS, size_t INPUTS, typename... Args>
DSPVectorArray<ROWS> mix_n(size_t inputIndex, DSPVectorArray<INPUTS> gains,
                           DSPVectorArray<ROWS> first, Args... args)
{
  return first * repeatRows<ROWS>(gains.getRowVectorUnchecked(inputIndex)) +
         mix_n(inputIndex + 1, gains, args...);
}

template <size_t ROWS, size_t INPUTS, typename... Args>
DSPVectorArray<ROWS> mix(DSPVectorArray<INPUTS> gains, DSPVectorArray<ROWS> first, Args... args)
{
  return mix_n(0, gains, first, args...);
}

// multiplex. selector is a signal that controls what mix of the inputs to send to the output.
// the selector range [0--1) is mapped to cover the range of inputs equally.

template <size_t ROWS, typename... Args>
DSPVectorArray<ROWS> multiplex(DSPVector selector, DSPVectorArray<ROWS> first, Args... args)
{
  DSPVectorArray<ROWS> inputs[]{first, args...};
  constexpr int nInputs = sizeof...(Args) + 1;

  DSPVectorArray<ROWS> y;

  // iterate on each sample of input selector
  for (int i = 0; i < kFloatsPerDSPVector * ROWS; ++i)
  {
    // TODO SIMD
    int selectorIdx = i % kFloatsPerDSPVector;
    float s = selector[selectorIdx];

    // get input index from 0 - nInputs-1
    float inputU = s - truncf(s);
    size_t inputSafe = inputU * nInputs;

    // read input sample to output
    y[i] = inputs[inputSafe][i];
  }
  return y;
}

// multiplex as above but with linear interpolation between inputs
// the selector range [0--1) is mapped so that 1.0 = the last input.

template <size_t ROWS, typename... Args>
DSPVectorArray<ROWS> multiplexLinear(DSPVector selector, DSPVectorArray<ROWS> first, Args... args)
{
  DSPVectorArray<ROWS> inputs[]{first, args...};
  constexpr int nInputs = sizeof...(Args) + 1;

  DSPVectorArray<ROWS> y;

  // iterate on each sample of input selector
  for (int i = 0; i < kFloatsPerDSPVector * ROWS; ++i)
  {
    // TODO SIMD
    int selectorIdx = i % kFloatsPerDSPVector;
    float s = selector[selectorIdx];

    // get input index from 0 - nInputs-1
    float inputU = s - truncf(s);
    float inputReal = inputU * nInputs;
    float inputInt = truncf(inputReal);
    float inputFrac = inputReal - inputInt;
    size_t input1Safe = inputInt;
    size_t input2Safe = (input1Safe + 1) % nInputs;

    // read input samples and interpolate to output
    y[i] = lerp(inputs[input1Safe][i], inputs[input2Safe][i], inputFrac);
  }
  return y;
}

// demultiplex the input to the outputs based on the value of the selector at each sample.

template <size_t ROWS, typename... Args>
void demultiplex(DSPVector selector, DSPVectorArray<ROWS> input, DSPVectorArray<ROWS>* firstOutput,
                 Args... args)
{
  DSPVectorArray<ROWS>* outputs[]{firstOutput, args...};
  constexpr int nOutputs = sizeof...(Args) + 1;

  DSPVector outputIntSafe;

  // for each sample, get the output index from the selector
  for (int i = 0; i < kFloatsPerDSPVector; ++i)
  {
    // TODO SIMD
    float s = selector[i];

    // get input index from 0 - nInputs-1
    float outputU = s - truncf(s);
    size_t outputInt = outputU * nOutputs;
    outputIntSafe[i] = outputInt;
  }

  // for each output, for each sample, if the selected output index at the
  // sample equals the output, write the input that that output. Else write 0.
  for (int j = 0; j < nOutputs; ++j)
  {
    DSPVectorArray<ROWS>* pOutput = outputs[j];
    for (int i = 0; i < kFloatsPerDSPVector * ROWS; ++i)
    {
      int selectorIdx = i % kFloatsPerDSPVector;
      size_t outputInt = outputIntSafe[selectorIdx];
      (*pOutput)[i] = (outputInt == j) ? input[i] : 0;
    }
  }
}

// demultiplex the input to the outputs based on the value of the selector at each sample.
// deinterpolate linearly to neighboring outputs.

template <size_t ROWS, typename... Args>
void demultiplexLinear(DSPVector selector, DSPVectorArray<ROWS> input,
                       DSPVectorArray<ROWS>* firstOutput, Args... args)
{
  DSPVectorArray<ROWS>* outputs[]{firstOutput, args...};
  constexpr int nOutputs = sizeof...(Args) + 1;

  DSPVector outputInt1Safe;
  DSPVector outputInt2Safe;
  DSPVector outputMix;

  // for each sample, get the two output indexes and mix amount from the selector
  for (int i = 0; i < kFloatsPerDSPVector; ++i)
  {
    // TODO SIMD
    float s = selector[i];

    // get input index from 0 - nInputs-1
    float outputU = s - truncf(s);
    float outputReal = outputU * nOutputs;
    float outputIntPart = truncf(outputReal);
    size_t outputInt = outputIntPart;
    outputMix[i] = outputReal - outputIntPart;
    outputInt1Safe[i] = outputInt;
    outputInt2Safe[i] = (outputInt + 1) % nOutputs;
  }

  // for each output, for each sample, if the selected output index at the
  // sample equals the output, write the input that that output. Else write 0.
  for (int j = 0; j < nOutputs; ++j)
  {
    DSPVectorArray<ROWS>* pOutput = outputs[j];
    for (int i = 0; i < kFloatsPerDSPVector * ROWS; ++i)
    {
      int selectorIdx = i % kFloatsPerDSPVector;
      size_t outputInt1 = outputInt1Safe[selectorIdx];
      size_t outputInt2 = outputInt2Safe[selectorIdx];
      float m = outputMix[selectorIdx];

      if (j == outputInt1)
      {
        // deinterpolate a
        (*pOutput)[i] = input[i] * (1.f - m);
      }
      else if (j == outputInt2)
      {
        // deinterpolate b
        (*pOutput)[i] = input[i] * m;
      }
      else
      {
        (*pOutput)[i] = 0;
      }
    }
  }
}

// should multiplex be on multiple inputs, rows of one input, different flavors for both??

// demultiplex(outputSelector, signalInput ) -> DSPVectorArray<inputs> ;

// splitRows(demultiplex(outputSelector, signalInput )

// REQUIRE(addRows(demultiplex(outputSelector, signalInput)) == signalInput);

}  // namespace ml
