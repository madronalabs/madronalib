// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <list>
#include <map>
#include <string>

#include "MLPath.h"
#include "MLSymbol.h"
#include "MLText.h"
#include "MLDSPProjections.h"

// Value is a tagged variant type for efficiently passing typed data (float, int, text, arrays,
// blobs) in DSP applications with minimal heap allocation. If the value to be constructed is below
// a certain size, no heap will be allocated. Whether on the stack or the heap, assigning a new
// value to one of the same size must not reallocate. Together these guarantees make Value useful
// for creating DSP applications that don't allocate memory when processing signals.

namespace ml
{

class Value
{
 public:
  enum Type
  {
    kUndefined = 0,
    kFloat,
    kText,
    kBlob,
    kFloatArray,
    kInt,
    kNumTypes
  };

  // copy and assign constructors and destructor
  Value(const Value& other);
  Value& operator=(const Value& other);
  Value(Value&& other) noexcept;
  Value& operator=(Value&& other) noexcept;
  ~Value();

  // Constructors with fixed-size data.
  Value();
  Value(float v);
  Value(double v);  // converts to float
  Value(int v);

  template <size_t N>
  explicit Value(std::array<float, N> values)
  {
    type_ = kFloatArray;
    auto listSize = N;
    sizeInBytes_ = static_cast<uint32_t>(listSize * sizeof(float));

    if (sizeInBytes_ <= kLocalDataBytes)
    {
      // store locally
      dataPtr_ = localData_;
      memcpy(dataPtr_, values.data(), sizeInBytes_);
    }
    else
    {
      // allocate heap
      dataPtr_ = (uint8_t*)malloc(sizeInBytes_);
      if (dataPtr_)
      {
        memcpy(dataPtr_, values.data(), sizeInBytes_);
      }
      else
      {
        dataPtr_ = localData_;
        sizeInBytes_ = 0;
        type_ = kUndefined;
      }
    }
  }

  // Constructors with variable-size data.
  Value(std::initializer_list<float> values);
  Value(const std::vector<float>& values);
  Value(const ml::Text& v);
  Value(const char* v);
  explicit Value(const uint8_t* data, size_t size);

  // Getters for fixed-size data. These convert scalar numeric types,
  // and return 0 where conversions don't make sense. That can lead to ambiguity, but
  // generally we know the types of Values we are using. Bool conversion is sometimes
  // handy even though bool is not an actual type we store.
  float getFloatValue() const;
  int getIntValue() const;
  bool getBoolValue() const;

  // Returns the float array if of type kFloatArray, otherwise an empty array.
  template <size_t N>
  std::array<float, N> getFloatArray() const
  {
    std::array<float, N> r;
    if (type_ == kFloatArray)
    {
      memcpy(r.data(), dataPtr_, sizeInBytes_);
    }
    return r;
  }

  // Getters for variable-size data.
  float* getFloatArrayPtr() const;
  size_t getFloatArraySize() const;
  std::vector<float> getFloatVector() const;
  ml::TextFragment getTextValue() const;

  // Null object check
  explicit operator bool() const;

  // Public utils
  bool isStoredLocally() const;
  bool operator==(const Value& b) const;
  bool operator!=(const Value& b) const;
  Type getType() const;
  const uint8_t* data() const;
  uint32_t size() const;

  // Static utils (for tests)
  static constexpr size_t kMaxDataSizeBits{28};
  static constexpr size_t kMaxDataBytes = 1 << (kMaxDataSizeBits - 1);
  static constexpr size_t getLocalDataMaxBytes() { return kLocalDataBytes; }
  static constexpr size_t getHeaderBytes() { return kHeaderBytes; }

 private:
  // kHeaderBytes should be the size of everything except the local data. This is verified in
  // valueTest.cpp.
  static constexpr size_t kStructSizeInBytes{64};
  static constexpr size_t kHeaderBytes{16};
  static constexpr size_t kLocalDataBytes = kStructSizeInBytes - kHeaderBytes;

  uint8_t* dataPtr_{localData_};
  Type type_{kUndefined};
  uint32_t sizeInBytes_{0};
  uint8_t localData_[kLocalDataBytes];

  // private utilities
  void copyOrAllocate(Type type, const uint8_t* pSrc, size_t bytes);
  void copyOrMove(Type newType, uint8_t* pSrc, size_t bytes);

  template <typename T>
  T toFixedSizeType() const
  {
    T* scalarTypePtr = reinterpret_cast<T*>(dataPtr_);
    return *scalarTypePtr;
  }

  // private constructor for deserialization
  Value(unsigned int type, unsigned int sizeInBytes, const uint8_t* dataPtr);

  // friend in MLSerialization
  friend Value readBinaryToValue(const uint8_t*& readPtr);
};

std::ostream& operator<<(std::ostream& out, const ml::Value& r);

// NamedValue is meant for initializing a list of Values from some data, whether in code
// or read in from a config file. Each incoming type must have only one way of being converted to a
// value. Floats and doubles are converted to floats. Bools and ints are converted to ints.
// Initializer lists full of various numeric types are converted to float arrays.

// toValue template - must be specialized for each supported type
template <typename T>
Value toValue(const T& val)
{
  static_assert(sizeof(T) == 0, "No toValue specialization exists for this type. ");
  return Value();
}

// Specializations for basic types. Using template specializations allows NamedValue to be extended
// to other types by code in other modules.

template <>
inline Value toValue<float>(const float& v)
{
  return Value(v);
}
template <>
inline Value toValue<double>(const double& v)
{
  return Value(v);
}
template <>
inline Value toValue<int>(const int& v)
{
  return Value(v);
}
template <>
inline Value toValue<bool>(const bool& v)
{
  return Value(v);
}
template <>
inline Value toValue<const char*>(const char* const& v)
{
  return Value(v);
}
template <>
inline Value toValue<TextFragment>(const TextFragment& v)
{
  return Value(v);
}

struct NamedValue
{
  ml::Path name;
  Value value;

  NamedValue(const char* np, Value nv) : name(runtimePath(np)), value(nv) {}

  template <typename T>
  NamedValue(const char* np, T nv) : name(runtimePath(np)), value(toValue(nv))
  {
  }

  // Microsoft's compiler didn't allow using std::initializer_list in the template, so we have this
  // additional constructor
  NamedValue(const char* np, std::initializer_list<float> list) : name(runtimePath(np)), value(list)
  {
  }
};

// Define a type for initializing a new object with a list of Values.
using WithValues = const std::initializer_list<NamedValue>;

// function for doing things when Values (or any other types) change
template <typename T>
inline bool valueChanged(T newValue, T& prevValue)
{
  if (newValue != prevValue)
  {
    prevValue = newValue;
    return true;
  }
  return false;
}

}  // namespace ml
