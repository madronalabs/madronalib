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
// transferred in messages.

namespace ml
{

// a Blob is a reference to some bytes of a given size somewhere. It doesn't own data.
struct Blob
{
  Blob() = default;
  explicit Blob( const uint8_t* data_, size_t size_ ) : data(data_), size(size_) {}
  const uint8_t* data{nullptr};
  size_t size{0};
};

class alignas(16) Value
{
public:
  
  enum Type
  {
    kUndefined = 0,
    kFloat,
    kDouble,
    kBool,
    kInt32,
    kInt64,
    kUInt32,
    kUInt64,
    kFloatArray,
    kDoubleArray,
    kText,
    kBlob
  };
    
private:
  
  static constexpr size_t kStructSizeInBytes{256};
  static constexpr size_t kLocalDataBytes = kStructSizeInBytes - sizeof(size_t) - sizeof(uint8_t *) - sizeof(Type);
  
  // data
  
  Type _type{kUndefined};
  size_t _sizeInBytes;
  uint8_t * _dataPtr{_localData};
  uint8_t _localData[kLocalDataBytes];
  
  // utilities
  
  bool isStoredLocally() const;
  void copyLocalData(const Value& other);
  void reallocateAndCopy(const Value& other);
  
  template<typename T> T toFixedSizeType() const
  {
    T* scalarTypePtr = reinterpret_cast<T*>(_dataPtr);
    return *scalarTypePtr;
  }

 public:

  // copy and assign constructors and destructor
  
  Value(const Value& other);
  Value& operator=(const Value& other);
//  Value (Value&& other) noexcept;
 // Value& operator=(Value&& other) noexcept;
  ~Value();
  
  // Constructors with fixed-size data.
  
  explicit Value();
  explicit Value(float v); 
  explicit Value(double v);
  explicit Value(bool v);
  Value(int32_t v);
  explicit Value(int64_t v);
  explicit Value(uint32_t v);
  explicit Value(uint64_t v);

  // Constructors with variable-size data.

  Value(std::initializer_list<float> values);
  Value(const char* v);
  explicit Value(const ml::Text& v);
  explicit Value(const ml::Blob& v);
  Value(std::vector<uint8_t> dataVec); // alternate Blob
  
  // getters for fixed-size data
  
  float getFloatValue() const;
  double getDoubleValue() const;
  bool getBoolValue() const;
  int32_t getInt32Value() const;
  int64_t getInt64Value() const;
  uint32_t getUInt32Value() const;
  uint64_t getUInt64Value() const;
  
  // getters for variable-size data.
  
  float* getFloatArrayPtr() const;
  size_t getFloatArraySize() const;
  std::vector<float> getFloatVector() const;
  double* getDoubleArrayPtr() const;
  size_t getDoubleArraySize() const;
  std::vector<double> getDoubleVector() const;
  ml::TextFragment getTextValue() const;
  ml::Blob getBlobValue() const;
  std::vector<uint8_t> getBlobVector() const;

  // public utils
  
  explicit operator bool() const;
  bool operator==(const Value& b) const;
  bool operator!=(const Value& b) const;
  Type getType() const;
};

std::ostream& operator<<(std::ostream& out, const ml::Value& r);

// NamedValue for initializer lists
struct NamedValue
{
  ml::Path name;
  Value value;

  template<typename T>
  NamedValue(ml::Path np, T nv) : name(np), value(Value(nv)) {}
  
  NamedValue(ml::Path np, std::initializer_list<float> values) : name(np), value(Value(values)) {}
};

// Define a type for initializing a new object with a list of Values.
using WithValues = const std::initializer_list< NamedValue >;

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
