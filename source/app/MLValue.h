// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <list>
#include <map>
#include <string>

#include "MLMatrix.h"
#include "MLPath.h"
#include "MLSymbol.h"
#include "MLText.h"
#include "MLDSPProjections.h"

// Value: a small unit of typed data designed for being constructed on the stack and
// transferred in messages. Values have the following types: undefined, float,
// text, blob, unsigned long, and matrix. (Matrix soon to be deprecated)

// TODO: instead of using Matrix directly here as a type, make a blob type
// and utilities (in Matrix) for conversion.


// functions needed for each convertible type:
// Value(type t);
// t get[Type]Value();
// explicit setValue(t);


namespace ml
{

static TextFragment kBlobHeader("!BLOB!");

class Value
{
 public:

  static constexpr size_t kLocalDataBytes{256};
  
  enum Type
  {
    kUndefinedValue = 0,
    kFloatValue,
    kFloatArrayValue,
    kTextValue,
    kBlobValue,
    kUnsignedLongValue
  };

  Value();
  Value(const Value& other);
  Value& operator=(const Value& other);
  Value(float v);
  Value(int v);
  Value(bool v);
  Value(long v);
  Value(unsigned long v);
  Value(unsigned long long v);
  Value(uint32_t v);
  Value(double v);
  Value(const ml::Text& t);
  Value(const char* t);
  Value(const float* i);

  // Blob constructors.
  // if data size > kBlobSizeBytes, blob values will allocate heap.
  explicit Value(const void* pData, size_t n);
  Value(const std::vector<uint8_t>& dataVec);
  // TODO make array-like ctor using gsl::span

  // matrix type constructor via initializer_list
  Value(std::initializer_list<float> values)
  {
    auto listSize = values.size();
    if (listSize == 0)
    {
      *this = Value();
    }
    else if (listSize == 1)
    {
      *this = Value(*values.begin());
    }
    else
    {
      constexpr size_t maxFloats = kLocalDataBytes / sizeof(float);
      size_t nFloats = std::min(maxFloats, listSize);
      mType = kFloatArrayValue;
      _blobSizeInBytes = nFloats*sizeof(float);
      float* pDest = reinterpret_cast<float*>(_localBlobData);
      for(const float& value : values)
      {
        pDest[0] = value;
        pDest++;
      }
    }
  }

  ~Value();

  inline const float getFloatValue() const { return mFloatVal; }
  inline const float getFloatValueWithDefault(float d) const
  {
    return (mType == kFloatValue) ? mFloatVal : d;
  }
  
  inline const float* getFloatArrayValue() const
  {
    if(mType == kFloatArrayValue)
    {
      float* pDest = reinterpret_cast<float*>(_localBlobData);
      return pDest;
    }
    return nullptr;
  }

  inline const float getBoolValue() const { return static_cast<bool>(mFloatVal); }

  inline const bool getBoolValueWithDefault(bool b) const
  {
    return (mType == kFloatValue) ? static_cast<bool>(mFloatVal) : b;
  }

  inline const int getIntValue() const { return static_cast<int>(mFloatVal); }

  inline const int getIntValueWithDefault(int d) const
  {
    return (mType == kFloatValue) ? static_cast<int>(mFloatVal) : d;
  }

  inline const uint32_t getUnsignedLongValue() const { return mUnsignedLongVal; }

  inline const uint32_t getUnsignedLongValueWithDefault(uint32_t d) const
  {
    return (mType == kUnsignedLongValue) ? mUnsignedLongVal : d;
  }

  inline const ml::Text getTextValue() const
  {
    return (mType == kTextValue) ? (mTextVal) : ml::Text();
  }

  inline const ml::Text getTextValueWithDefault(Text d) const
  {
    return (mType == kTextValue) ? (mTextVal) : d;
  }
  
  inline void* getBlobData() const
  {
    if (mType == kBlobValue)
    {
      return (void*)pBlobData;
    }
    else
    {
      return nullptr;
    }
  }

  inline size_t getBlobSize() const
  {
    if (mType == kBlobValue)
    {
      return _blobSizeInBytes;
    }
    else
    {
      return 0;
    }
  }
  
  inline std::vector<uint8_t> getBlobValue() const
  {
    if (mType == kBlobValue)
    {
      return std::vector<uint8_t>(pBlobData, pBlobData + _blobSizeInBytes);
    }
    else
    {
      return std::vector<uint8_t>();
    }
  }

  // For each type of property, a setValue method must exist
  // to set the value of the property to that of the argument.
  //
  // For each type of property, if the size of the argument is equal to the
  // size of the current value, the value must be modified in place.
  // This guarantee keeps DSP graphs from allocating memory as they run.
  void setValue(const Value& v);
  void setValue(const float& v);
  void setValue(const int& v);
  void setValue(const bool& v);
  void setValue(const uint32_t& v);
  void setValue(const long& v);
  void setValue(const double& v);
  void setValue(const ml::Text& v);
  void setValue(const char* const v);
  void setValue(const Interval v);

  explicit operator bool() const { return (mType != kUndefinedValue); }

  bool operator==(const Value& b) const;
  bool operator!=(const Value& b) const;

  Type getType() const { return mType; }
  Symbol getTypeAsSymbol() const
  {
    static Symbol kTypesAsSymbols[4]{"undefined", "float", "text", "matrix"};
    return kTypesAsSymbols[mType];
  }
  bool isUndefinedType() { return mType == kUndefinedValue; }
  bool isFloatType() { return mType == kFloatValue; }
  bool isTextType() { return mType == kTextValue; }

  bool operator<<(const Value& b) const;

 private:
  void copyBlob(const void* inputData, size_t size);

  uint8_t _localBlobData[kLocalDataBytes];
  size_t _blobSizeInBytes;
  uint8_t *pBlobData{_localBlobData};

  // TODO reduce storage requirements and reduce copying!
  // this is a minimal-code start.
  // everything will share space.
  Type mType{kUndefinedValue};
  float mFloatVal{};
  ml::Text mTextVal{};
  uint32_t mUnsignedLongVal{};
  Interval mIntervalVal{};
};

// NamedValue for initializer lists
struct NamedValue
{
  ml::Path name{};
  Value value{};

  NamedValue() = default;
  NamedValue(ml::Path np, Value nv) : name(np), value(nv) {}
};

// Define a type for initializing a new object with a list of Values.
using WithValues = const std::initializer_list< NamedValue >;

std::string getTypeDebugStr(const ml::Value& r);
std::ostream& operator<<(std::ostream& out, const ml::Value& r);

// template function for doing things when Values (or any other types) change
template<typename T>
inline bool valueChanged(T newValue, T& prevValue)
{
  if(newValue != prevValue)
  {
    prevValue = newValue;
    return true;
  }
  return false;
}

}  // namespace ml
