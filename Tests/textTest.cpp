// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <numeric>
#include <thread>
#include <unordered_map>
#include <vector>

#include "MLTextUtils.h"
#include "catch.hpp"
#include "madronalib.h"


using namespace ml;

TEST_CASE("madronalib/core/text", "[text]")
{
  const char* kobayashi("\xE5\xB0\x8F\xE6\x9E\x97\x20\xE5\xB0\x8A");

  TextFragment t0("/////");
  TextFragment t1("a/");
  TextFragment t2("////a/b");
  TextFragment t3("a/b/c/////");
  TextFragment t4("hello/world/get//segments");
  TextFragment t5("///hello/world/get///", kobayashi, "/segments");

  std::vector< TextFragment > tv{t0, t1, t2, t3, t4, t5};
  
  int i{0};
  for(auto tf : tv)
  {
    auto segs = textUtils::split(tf, '/');
    REQUIRE(segs.size() == i++);
    
    Path p = textToPath(tf);
    TextFragment tf2 = pathToText(p);
    Path p2 = textToPath(tf2);
    
    REQUIRE(p == p2);
  }

  for(int i=0; i<100; ++i)
  {
    auto t = textUtils::naturalNumberToText(i);
    auto i2 = textUtils::textToNaturalNumber(t);
    REQUIRE(i2 == i);
  }


  std::vector<TextFragment> testVec{"10203.f", "0", "", kobayashi, "a/b/c"};
  for(auto t : testVec)
  {
    auto tPts = textToCodePoints(t);
    auto t2 = codePointsToText(tPts);
    auto eq = (t == t2);
    REQUIRE(t == t2);
  }
}

