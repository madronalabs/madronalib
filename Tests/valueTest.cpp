// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "madronalib.h"

using namespace ml;

namespace
{
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
}

TEST_CASE("madronalib/core/values/core", "[values]")
{
  // Memory layout
  Value v;
  auto valueHeaderPtr = reinterpret_cast<char*>(&v);
  auto valueDataPtr = reinterpret_cast<const char*>(v.data());
  size_t dataOffsetBytes = valueDataPtr - valueHeaderPtr;
  REQUIRE(dataOffsetBytes == Value::getHeaderBytes());
  
  // Value converters to and from small types (no type checking!)
  testType tv1{3, 4, 5, {3.4, 3.5, 3.6}};
  auto tv1Val = smallTypeToValue<testType>(tv1);
  testType tv2 = valueToSmallType<testType>(tv1Val);
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
  
  // constructors
  Value floatVal (27.5f);
  REQUIRE(floatVal.getType() == Value::kFloat);
  
  Value floatArrayVal {27.5, 55., 110., 220., 440., 880., 1760.};
  REQUIRE(floatArrayVal.getType() == Value::kFloatArray);
  
}

TEST_CASE("madronalib/core/values/rule_of_five", "[values]")
{
  SECTION("copy constructor - local storage")
  {
    Value v1(42.5f);
    Value v2(v1);
    REQUIRE(v2.getFloatValue() == 42.5f);
    REQUIRE(v2.isStoredLocally());
    
    // Verify independence
    v1 = 100.0f;
    REQUIRE(v2.getFloatValue() == 42.5f);
  }
  
  SECTION("copy constructor - heap storage")
  {
    std::vector<float> bigVec(200, 3.14f);
    Value v1(bigVec);
    Value v2(v1);
    
    REQUIRE(!v1.isStoredLocally());
    REQUIRE(!v2.isStoredLocally());
    REQUIRE(v2.getFloatArraySize() == 200);
    REQUIRE(v2.getFloatArrayPtr()[0] == 3.14f);
    
    // Different heap allocations
    REQUIRE(v1.data() != v2.data());
  }
  
  SECTION("copy assignment - same size reuses storage")
  {
    Value v1(42.5f);
    Value v2(99.9f);
    
    auto oldPtr = v2.data();
    v2 = v1;
    
    REQUIRE(v2.getFloatValue() == 42.5f);
    REQUIRE(v2.data() == oldPtr); // Should reuse storage
  }
  
  SECTION("copy assignment - local to heap")
  {
    Value v1(42.5f);
    std::vector<float> bigVec(200, 7.7f);
    Value v2(bigVec);
    
    v2 = v1; // Heap should be freed, now local
    
    REQUIRE(v2.getFloatValue() == 42.5f);
    REQUIRE(v2.isStoredLocally());
  }
  
  SECTION("move constructor - heap storage")
  {
    std::vector<float> bigVec(200, 2.5f);
    Value v1(bigVec);
    auto oldPtr = v1.data();
    
    Value v2(std::move(v1));
    
    REQUIRE(v2.getFloatArraySize() == 200);
    REQUIRE(v2.data() == oldPtr); // Stole the pointer
    REQUIRE(v1.getType() == Value::kUndefined); // Moved-from state
    REQUIRE(v1.isStoredLocally()); // Reset to safe state
  }
  
  SECTION("move assignment - heap to heap")
  {
    std::vector<float> vec1(200, 1.1f);
    std::vector<float> vec2(300, 2.2f);
    Value v1(vec1);
    Value v2(vec2);
    
    auto ptr1 = v1.data();
    v2 = std::move(v1);
    
    REQUIRE(v2.getFloatArraySize() == 200);
    REQUIRE(v2.data() == ptr1); // Stole v1's pointer
    REQUIRE(v1.getType() == Value::kUndefined);
  }
  
  SECTION("self-assignment")
  {
    Value v1(42.5f);
    v1 = v1;
    REQUIRE(v1.getFloatValue() == 42.5f);
    
    Value v2(42.5f);
    v2 = std::move(v2);
    REQUIRE(v2.getFloatValue() == 42.5f);
  }
}

TEST_CASE("madronalib/core/values/type_conversions", "[values]")
{
  SECTION("getFloatValue converts from other numeric types")
  {
    Value intVal(42);
    REQUIRE(intVal.getFloatValue() == 42.0f);
    
    Value doubleVal(3.14);
    REQUIRE(doubleVal.getFloatValue() == Approx(3.14f));
  }
  
  SECTION("getFloatValue returns 0 for incompatible types")
  {
    Value textVal("hello");
    REQUIRE(textVal.getFloatValue() == 0.0f);
    
    Value arrayVal{1.0f, 2.0f, 3.0f};
    REQUIRE(arrayVal.getFloatValue() == 0.0f);
  }
  
  SECTION("getTextValue returns empty for non-text")
  {
    Value floatVal(42.5f);
    auto text = floatVal.getTextValue();
    REQUIRE(text.lengthInBytes() == 0);
  }
}

TEST_CASE("madronalib/core/values/equality", "[values]")
{
  SECTION("same type and value")
  {
    Value v1(42.5f);
    Value v2(42.5f);
    REQUIRE(v1 == v2);
    REQUIRE(!(v1 != v2));
  }
  
  SECTION("different types")
  {
    Value floatVal(42.0f);
    Value intVal(42);
    REQUIRE(floatVal != intVal); // Different types, even if semantically similar
  }
  
  SECTION("different values")
  {
    Value v1(42.5f);
    Value v2(99.9f);
    REQUIRE(v1 != v2);
  }
  
  SECTION("arrays")
  {
    Value v1({1.0f, 2.0f, 3.0f});
    Value v2({1.0f, 2.0f, 3.0f});
    Value v3({1.0f, 2.0f, 3.1f});
    
    REQUIRE(v1 == v2);
    REQUIRE(v1 != v3);
  }
  
  SECTION("undefined values")
  {
    Value v1;
    Value v2;
    REQUIRE(v1 == v2);
  }
}

TEST_CASE("madronalib/core/values/bool_conversion", "[values]")
{
  SECTION("undefined is false")
  {
    Value v;
    REQUIRE(!v);
    REQUIRE(static_cast<bool>(v) == false);
  }
  
  SECTION("defined values are true")
  {
    Value v1(0.0f);
    Value v2(42);
    Value v3("text");
    
    REQUIRE(v1); // Even zero is "defined"
    REQUIRE(v2);
    REQUIRE(v3);
  }
}

TEST_CASE("madronalib/core/values/text_and_blobs", "[values]")
{
  SECTION("char* constructor")
  {
    Value v("hello world");
    REQUIRE(v.getType() == Value::kText);
    auto text = v.getTextValue();
    REQUIRE(std::string(text.getText(), text.lengthInBytes()) == "hello world");
  }
  
  SECTION("ml::Text constructor")
  {
    ml::Text t("testing");
    Value v(t);
    REQUIRE(v.getType() == Value::kText);
  }
  
  SECTION("vector<uint8_t> constructor")
  {
    std::vector<uint8_t> vec{10, 20, 30, 40};
    Value v(vec.data(), vec.size());
    
    REQUIRE(v.getType() == Value::kBlob);
    std::vector<uint8_t> retrieved (v.data(), v.data() + v.size());
    REQUIRE(retrieved == vec);
  }
}

TEST_CASE("madronalib/core/values/edge_cases", "[values]")
{
  SECTION("empty text")
  {
    Value v("");
    REQUIRE(v.getType() == Value::kText);
    REQUIRE(v.size() == 0);
  }
  
  SECTION("empty float array")
  {
    std::vector<float> empty;
    Value v(empty);
    REQUIRE(v.getType() == Value::kFloatArray);
    REQUIRE(v.getFloatArraySize() == 0);
  }
  
  SECTION("single element array")
  {
    Value v({42.0f});
    REQUIRE(v.getFloatArraySize() == 1);
    REQUIRE(v.getFloatArrayPtr()[0] == 42.0f);
  }
  
  SECTION("nullptr getters for wrong type")
  {
    Value intVal(42);
    REQUIRE(intVal.getFloatArrayPtr() == nullptr);
    REQUIRE(intVal.getFloatArraySize() == 0);
  }
}
