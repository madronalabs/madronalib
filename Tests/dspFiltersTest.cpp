// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include <chrono>
#include <iostream>
#include <map>
#include <thread>
#include <unordered_map>
#include <vector>

#include "catch.hpp"
#include "madronalib.h"
#include "mldsp.h"
#include "tests.h"
#include "MLDSPSample.h"
#include "MLDSPFilters.h"

using namespace ml;

TEST_CASE("madronalib/core/dsp_filters", "[dsp_filters]")
{
  constexpr int kOctaves{3};
  Upsampler upper(kOctaves);
  Downsampler downer(kOctaves);

  const int kSamples{300};
  Sample sine1;
  Sample sine1Upsampled;
  Sample sine2;

  size_t framesToProcess{0};
  if(!resize(sine1, kSamples)) return;
  if(!resize(sine1Upsampled, kSamples << kOctaves)) return;

  float dw = kTwoPi/2.0f/kFloatsPerDSPVector;
  float omega{0.f};
  framesToProcess = getSize(sine1);
  
  // TODO functional-style fill of Sample, and process loop
  for(int i=0; i<framesToProcess; ++i)
  {
    sine1[i] = sinf(omega);
    omega += dw;
  }
  
  // process until we run out of whole DSPVectors - there may be some frames left over
  size_t frameIdx{0};
  while(frameIdx + kFloatsPerDSPVector <= framesToProcess)
  {
    DSPVector vIn;
    auto pSrc = getConstFramePtr(sine1, frameIdx);
    load(vIn, pSrc);
    
    frameIdx += kFloatsPerDSPVector;

    upper.write(vIn);
    
    for(int i=0; i < (1 << kOctaves); ++i)
    {
      DSPVector vOut = upper.read();
      bool r = downer.write(vOut);
    }
    
    // TODO real tests, measure delay?
    DSPVector sineOut = downer.read();
  }
}
