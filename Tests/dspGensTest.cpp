// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "testUtils.h"
#include "MLDSPGens.h"

using namespace ml;

TEST_CASE("madronalib/core/dsp_gens", "[dsp_gens]")
{
  PhasorGen p1;
  p1.clear();
  auto v0 = p1(1.f/kFloatsPerDSPVector);
  
  
//  std::cout << "phasor: " << v0 << "\n";
  
  SineGen s1;
  s1.clear();
  auto v1 = s1(1.f/kFloatsPerDSPVector);
  
  
//  std::cout << "sine: " << v1 << "\n";
  
  // one cycle of sine wave should end at 0
  float epsilon = dBToAmp(-120.f);
  REQUIRE(fabs(v1[kFloatsPerDSPVector - 1]) < epsilon);
  
  // one shot
  OneShotGen g1;
  auto vg0 = g1(1.f/kFloatsPerDSPVector);
  
  g1.trigger();
  auto vg1 = g1(1.f/kFloatsPerDSPVector);
  auto vg2 = g1(1.f/kFloatsPerDSPVector);

  
//  std::cout << "0: " << vg0 << "\n";
//  std::cout << "1: " << vg1 << "\n";
//  std::cout << "2: " << vg2 << "\n";

  
}
