// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/



#pragma once

#ifdef _WIN32
#include <memory>
#else
//#include <tr1/memory>
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

template <size_t ROWS, size_t INPUTS, typename ... Args>
DSPVectorArray<ROWS> mix_n(size_t inputIndex, DSPVectorArray<INPUTS> gains, DSPVectorArray<ROWS> first)
{
  return first*repeatRows<ROWS>(gains.getRowVectorUnchecked(inputIndex));
}

template <size_t ROWS, size_t INPUTS, typename ... Args>
DSPVectorArray<ROWS> mix_n(size_t inputIndex, DSPVectorArray<INPUTS> gains, DSPVectorArray<ROWS> first, Args... args)
{
  return first*repeatRows<ROWS>(gains.getRowVectorUnchecked(inputIndex)) + mix_n(inputIndex + 1, gains, args...);
}

template <size_t ROWS, size_t INPUTS, typename ... Args>
DSPVectorArray<ROWS> mix(DSPVectorArray<INPUTS> gains, DSPVectorArray<ROWS> first, Args... args)
{
  return mix_n(0, gains, first, args...);
}



// multiplex. selector is a signal that controls what mix of the inputs to send to the output.
// the selector range [0--1) is mapped to cover the range of inputs.

template <size_t ROWS, typename... Args>
DSPVectorArray<ROWS> multiplex(DSPVector selector, DSPVectorArray<ROWS> first, Args ... args)
{
  DSPVectorArray<ROWS> inputs[] {first, args...};
  constexpr int nInputs = sizeof...(Args) + 1;
  
  DSPVectorArray<ROWS> y;
  
  // iterate on each sample of input selector
  for (int i = 0; i < kFloatsPerDSPVector*ROWS; ++i)
  {
    // TODO SIMD
    int selectorIdx = i%kFloatsPerDSPVector;
    float s = selector[selectorIdx];
    float inputU = s - truncf(s);
    size_t inputSafe = (inputU * nInputs);
    
    // TODO mix

    y[i] = inputs[inputSafe][i];
  }
  return y;
}

 
 /*
template <size_t ROWS, size_t INPUTS, typename ... Args>
DSPVectorArray<ROWS> multiplex(DSPVector selector, DSPVectorArray<ROWS> first, Args... args)
{
  // TODO make some reasonable assumptions about how fast the selector signal may be
  // changing, and optimize
  
  DSPVectorArray<ROWS> y;
  
  // iterate on each sample of input selector
  for (int i = 0; i < kFloatsPerDSPVector; ++i)
  {
    float s = selector[i];
    size_t input = truncf(s);
    size_t input1Safe = (input % INPUTS);
    size_t input2Safe = (input + 1) % INPUTS;
    float inputFrac = s - inputInt;
    
    
    // iterate on each sample of inputs
    for (int j = 0; j < kFloatsPerDSPVector * ROWS; ++j)
    {
      y[j] = lerp(input1Safe, input2Safe, inputFrac);
    }

  }

  return mix_n(0, gains, first, args...);
}
*/

// should multiplex be on multiple inputs, rows of one input, different flavors for both?? 


// demultiplex(outputSelector, signalInput ) -> DSPVectorArray<inputs> ;

// splitRows(demultiplex(outputSelector, signalInput )

// REQUIRE(addRows(demultiplex(outputSelector, signalInput)) == signalInput);



}  // namespace ml
