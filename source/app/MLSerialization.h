// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// converters to/from binary and text formats for various objects.

#pragma once

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <numeric>

#include "MLSymbol.h"
#include "MLText.h"
#include "MLTextUtils.h"
#include "MLTree.h"
#include "MLValue.h"

namespace ml
{

static TextFragment kBlobHeader("!BLOB!");

// Values

// Return size of the binary representation of the Value (including header)
size_t getBinarySize(const Value& v);

// Return the binary representation of the Value.
std::vector<uint8_t> valueToBinary(Value v);

// Return the Value represented by the vector of bytes.
Value binaryToValue(const std::vector<uint8_t>& v);

// Write the binary representation of the Value and increment the write pointer.
void writeValueToBinary(Value v, uint8_t*& writePtr);

// Read the binary representation of the Value and increment the read pointer.
Value readBinaryToValue(const uint8_t*& readPtr);

// Paths

std::vector<unsigned char> pathToBinary(Path p);
Path binaryDataToPath(const unsigned char* p);
Path binaryToPath(const std::vector<unsigned char>& p);

// Value Trees

std::vector<unsigned char> valueTreeToBinary(const Tree<Value>& t);
Tree<Value> binaryToValueTree(const std::vector<unsigned char>& binaryData);

// JSON
// utility class to make the cJSON interface usable with RAII style.
class JSONHolder
{
 public:
  JSONHolder();
  ~JSONHolder();

  // JSONHolders can't be copied.
  JSONHolder(const JSONHolder& b) = delete;
  JSONHolder& operator=(const JSONHolder&) = delete;

  // Allow move operations.
  JSONHolder(JSONHolder&&) noexcept;
  JSONHolder& operator=(JSONHolder&&) noexcept;

  struct Impl;
  Impl* pImpl{nullptr};

  void addNumber(TextFragment key, double number);
  void addString(TextFragment key, const char* str);
  void addFloatVector(TextFragment key, std::vector<float>& v);

  // transfer ownership of the data in j to this object, modifying j such that
  // when it goes out of scope nothing will be deleted.
  void addJSON(TextFragment key, JSONHolder& j);
};

// return a JSON object representing the value tree.
// The caller is responsible for freeing the JSONHolder object.
JSONHolder valueTreeToJSON(const Tree<Value>& t);

Tree<Value> JSONToValueTree(const JSONHolder& root);

// return a JSON object described by the text.
// The caller is responsible for freeing the JSONHolder object.
JSONHolder textToJSON(TextFragment t);

TextFragment JSONToText(const JSONHolder& root);

}  // namespace ml
