// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "madronalib.h"

using namespace ml;


struct testType
{
  float a;
  int b;
  double c;
  std::array<float, 3> d;
};

bool operator==(testType l, testType r)
{
  return (l.a == r.a) && (l.b == r.b) && (l.c == r.c);
}

TEST_CASE("madronalib/core/values", "[values]")
{
  // Memory layout
  Value v;
  auto valueHeaderPtr = reinterpret_cast<char*>(&v);
  auto valueDataPtr = reinterpret_cast<char*>(v.data());
  size_t dataOffsetBytes = valueDataPtr - valueHeaderPtr;
  REQUIRE(dataOffsetBytes == Value::getHeaderBytes());
  
  // Value converters to and from POD types
  testType tv1{3, 4, 5, {34, 35, 36}};
  auto tv1Val = valueFromPODType<testType>(tv1);
  testType tv2 = valueToPODType<testType>(tv1Val);
  REQUIRE(tv1 == tv2);
  
  // std::array<float> converters
  std::array<float, 5> far1{2, 4, 3, 2, 9};
  Value far1Val (far1);
  auto far2 = far1Val.getFloatArray<5>();
  REQUIRE(far1 == far2);
  
  // no heap guarantee
  std::vector<float> smallVec{1, 2, 3, 4};
  std::vector<float> maxLocalVec;
  maxLocalVec.resize(Value::getLocalDataMaxBytes()/sizeof(float));
  std::vector<float> aBitLargerVec;
  aBitLargerVec.resize(Value::getLocalDataMaxBytes()/sizeof(float) + 1);
  Value v1(smallVec);
  Value v2(maxLocalVec);
  Value v3(aBitLargerVec);
  REQUIRE(v1.isStoredLocally());
  REQUIRE(v2.isStoredLocally());
  REQUIRE(!v3.isStoredLocally());
  
}

