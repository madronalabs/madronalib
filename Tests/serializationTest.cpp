// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
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


TEST_CASE("madronalib/core/serialization", "[serialization]")
{
  theSymbolTable().clear();
  


  // Value tree to JSON to value tree. NOTE: the JSON created does not reflect the
  // tree structure but rather a flat list with the whole path as each item's string. TODO fix.
  Tree<Value> v;
  v["a"] = 0.4f;
  v["b"] = "hello";
  v["c"] = "hello";
  v["a/a"] = "hello";
  v["a/b"] = "hello";
  v["floatarr"] = Value(std::array<float, 5>{1.2, 3.4, 5.5, 23.4, -0.000000001});

  v["a/b/c"] = "hello again";
  v["b/q"] = "goodbye";
  v["q/q"] = "goodbye";
  v["q"] = 0.3;
  v["quizzle"] = 0.4;
  v["shizzle"] = 0.5;
  v["bizzle"] = 0.6;
  std::vector<uint8_t> someData{1, 3, 5, 7, 9};
  v["blobtest"] = Value(someData.data(), someData.size());
    
  // test each value in the tree separately
  for(auto treeVal : v)
  {
    auto b = valueToBinary(treeVal);
    auto treeVal2 = binaryToValue(b);
    REQUIRE(treeVal == treeVal2);
  }
  
  v.dump();
  
  std::cout << "-----------------------\n";
  
  theSymbolTable().dump();
  
  Tree< Value > v2 = JSONToValueTree(valueTreeToJSON(v));
  REQUIRE(v == v2);
  
  // Value tree to JSON to text to JSON to value tree.
  auto t1 = JSONToText(valueTreeToJSON(v));
  auto v3 = JSONToValueTree(textToJSON(t1));
  REQUIRE(v == v3);
  
  // a tree converted to binary and back should result in the original value

  auto b = valueTreeToBinary(v);
  auto vv = binaryToValueTree(b);
  
  std::cout << "vv\n";
  vv.dump();
  auto b2 = valueTreeToBinary(vv);
  
  // TEMP
  // TODO fix with real Tree keys
//  REQUIRE(b == b2);

  
  // create some JSON directly using our minimal API
  auto j5 = JSONHolder();
  j5.addNumber("foo", 23.0);
  auto j6 = JSONHolder();
  j6.addNumber("bar", 24.0);
  
  // addJSON() transfers the ownership of the data in j6 to j5
  j5.addJSON("j-obj", j6);
  REQUIRE(JSONToValueTree(j5).size() == 2);
  REQUIRE(JSONToValueTree(j6).size() == 0);
}

TEST_CASE("madronalib/core/value_serialization", "[serialization][values]")
{
  SECTION("round-trip float")
  {
    Value v1(42.5f);
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    REQUIRE(v2.getType() == Value::kFloat);
    REQUIRE(v2.getFloatValue() == 42.5f);
    REQUIRE(v1 == v2);
  }
  
  SECTION("round-trip int")
  {
    Value v1(12345);
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    REQUIRE(v2.getType() == Value::kInt);
    REQUIRE(v2.getIntValue() == 12345);
    REQUIRE(v1 == v2);
  }

  
  SECTION("round-trip text")
  {
    Value v1("Hello, World!");
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    REQUIRE(v2.getType() == Value::kText);
    auto text = v2.getTextValue();
    REQUIRE(std::string(text.getText(), text.lengthInBytes()) == "Hello, World!");
    REQUIRE(v1 == v2);
  }
  
  SECTION("round-trip empty text")
  {
    Value v1("");
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    REQUIRE(v2.getType() == Value::kText);
    REQUIRE(v2.size() == 0);
    REQUIRE(v1 == v2);
  }
  
  SECTION("round-trip blob")
  {
    std::vector<uint8_t> data{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};
    Value v1(data.data(), data.size());
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    REQUIRE(v2.getType() == Value::kBlob);
    
    std::vector<uint8_t> data2(v2.data(), v2.data() + v2.size());
    REQUIRE(data2.size() == 6);
    REQUIRE(std::memcmp(data2.data(), data.data(), 6) == 0);
    REQUIRE(v1 == v2);
  }
  
  SECTION("round-trip float array - local storage")
  {
    Value v1({1.1f, 2.2f, 3.3f, 4.4f, 5.5f});
    REQUIRE(v1.isStoredLocally());
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    REQUIRE(v2.getType() == Value::kFloatArray);
    REQUIRE(v2.getFloatArraySize() == 5);
    auto arr = v2.getFloatVector();
    REQUIRE(arr == std::vector<float>{1.1f, 2.2f, 3.3f, 4.4f, 5.5f});
    REQUIRE(v1 == v2);
  }
  
  SECTION("round-trip float array - heap storage")
  {
    std::vector<float> bigArray(200, 7.77f);
    Value v1(bigArray);
    REQUIRE(!v1.isStoredLocally());
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    REQUIRE(v2.getType() == Value::kFloatArray);
    REQUIRE(v2.getFloatArraySize() == 200);
    REQUIRE(v2.getFloatArrayPtr()[0] == 7.77f);
    REQUIRE(v2.getFloatArrayPtr()[199] == 7.77f);
    REQUIRE(v1 == v2);
  }
  
  SECTION("round-trip std::array<float>")
  {
    std::array<float, 4> arr{440.0f, 880.0f, 1760.0f, 3520.0f};
    Value v1(arr);
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    auto arr2 = v2.getFloatArray<4>();
    REQUIRE(arr == arr2);
  }
  
  SECTION("round-trip undefined")
  {
    Value v1; // undefined
    
    size_t binarySize = getBinarySize(v1);
    std::vector<uint8_t> buffer(binarySize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value v2 = readBinaryToValue(readPtr);
    
    REQUIRE(v2.getType() == Value::kUndefined);
    REQUIRE(!v2); // bool conversion
    REQUIRE(v1 == v2);
  }
  
  SECTION("multiple values in sequence")
  {
    Value v1(42.5f);
    Value v2("test string");
    Value v3({1.0f, 2.0f, 3.0f});
    Value v4(123);
    
    size_t totalSize = getBinarySize(v1) + getBinarySize(v2) +
    getBinarySize(v3) + getBinarySize(v4);
    std::vector<uint8_t> buffer(totalSize);
    uint8_t* writePtr = buffer.data();
    
    writeValueToBinary(v1, writePtr);
    writeValueToBinary(v2, writePtr);
    writeValueToBinary(v3, writePtr);
    writeValueToBinary(v4, writePtr);
    
    const uint8_t* readPtr = buffer.data();
    Value r1 = readBinaryToValue(readPtr);
    Value r2 = readBinaryToValue(readPtr);
    Value r3 = readBinaryToValue(readPtr);
    Value r4 = readBinaryToValue(readPtr);
    
    REQUIRE(r1 == v1);
    REQUIRE(r2 == v2);
    REQUIRE(r3 == v3);
    REQUIRE(r4 == v4);
    
    // Verify read pointer advanced correctly
    REQUIRE(readPtr == buffer.data() + totalSize);
  }
  
  SECTION("getBinarySize matches actual serialized size")
  {
    Value floatVal(42.5f);
    Value textVal("testing");
    std::vector<float> bigVec(100, 1.0f);
    Value arrayVal(bigVec);
    
    // Verify getBinarySize() returns the correct size for serialization
    std::vector<uint8_t> buffer1(getBinarySize(floatVal));
    uint8_t* writePtr1 = buffer1.data();
    writeValueToBinary(floatVal, writePtr1);
    REQUIRE(writePtr1 - buffer1.data() == getBinarySize(floatVal));
    
    std::vector<uint8_t> buffer2(getBinarySize(textVal));
    uint8_t* writePtr2 = buffer2.data();
    writeValueToBinary(textVal, writePtr2);
    REQUIRE(writePtr2 - buffer2.data() == getBinarySize(textVal));
    
    std::vector<uint8_t> buffer3(getBinarySize(arrayVal));
    uint8_t* writePtr3 = buffer3.data();
    writeValueToBinary(arrayVal, writePtr3);
    REQUIRE(writePtr3 - buffer3.data() == getBinarySize(arrayVal));
  }
  

  SECTION("write pointer advances correctly")
  {
    Value v1(42.5f);
    Value v2(99);
    
    std::vector<uint8_t> buffer(1000);
    uint8_t* startPtr = buffer.data();
    uint8_t* writePtr = startPtr;
    
    writeValueToBinary(v1, writePtr);
    size_t offset1 = writePtr - startPtr;
    REQUIRE(offset1 == getBinarySize(v1));
    
    writeValueToBinary(v2, writePtr);
    size_t offset2 = writePtr - startPtr;
    REQUIRE(offset2 == getBinarySize(v1) + getBinarySize(v2));
  }
  
  SECTION("read pointer advances correctly")
  {
    Value v1(1.1f);
    Value v2(2.2f);
    Value v3(3.3f);
    
    std::vector<uint8_t> buffer(1000);
    uint8_t* writePtr = buffer.data();
    writeValueToBinary(v1, writePtr);
    writeValueToBinary(v2, writePtr);
    writeValueToBinary(v3, writePtr);
    
    const uint8_t* startPtr = buffer.data();
    const uint8_t* readPtr = startPtr;
    
    Value r1 = readBinaryToValue(readPtr);
    REQUIRE(readPtr - startPtr == getBinarySize(v1));
    
    Value r2 = readBinaryToValue(readPtr);
    REQUIRE(readPtr - startPtr == getBinarySize(v1) + getBinarySize(v2));
    
    Value r3 = readBinaryToValue(readPtr);
    REQUIRE(readPtr - startPtr == getBinarySize(v1) + getBinarySize(v2) + getBinarySize(v3));
  }
}

TEST_CASE("madronalib/core/value_serialization/stress", "[serialization][values]")
{
  SECTION("many small values")
  {
    const int count = 1000;
    std::vector<Value> values;
    
    for(int i = 0; i < count; ++i) {
      values.push_back(Value(static_cast<float>(i)));
    }
    
    // Calculate total size
    size_t totalSize = 0;
    for(const auto& v : values) {
      totalSize += getBinarySize(v);
    }
    
    // Serialize all
    std::vector<uint8_t> buffer(totalSize);
    uint8_t* writePtr = buffer.data();
    for(const auto& v : values) {
      writeValueToBinary(v, writePtr);
    }
    
    // Deserialize all
    const uint8_t* readPtr = buffer.data();
    for(int i = 0; i < count; ++i) {
      Value v = readBinaryToValue(readPtr);
      REQUIRE(v.getFloatValue() == static_cast<float>(i));
    }
  }
  
  SECTION("mixed types in sequence")
  {
    std::vector<uint8_t> blob1{0xAA, 0xBB};
    std::vector<Value> values{
      Value(1.5f),
      Value(42),
      Value("test"),
      Value({1.0f, 2.0f}),
      Value(true),
      Value(3.14159),
      Value(blob1.data(), blob1.size()),
      Value()  // undefined
    };
    
    size_t totalSize = 0;
    for(const auto& v : values) {
      totalSize += getBinarySize(v);
    }
    
    std::vector<uint8_t> buffer(totalSize);
    uint8_t* writePtr = buffer.data();
    for(const auto& v : values) {
      writeValueToBinary(v, writePtr);
    }
    
    const uint8_t* readPtr = buffer.data();
    for(size_t i = 0; i < values.size(); ++i) {
      Value v = readBinaryToValue(readPtr);
      REQUIRE(v == values[i]);
    }
  }
}


