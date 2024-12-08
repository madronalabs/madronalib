// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <list>
#include <map>
#include <string>

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

class Value
{
public:
  
  enum Type
  {
    kUndefined = 0,
    kFloat,
    kDouble,
    kBool,
    kInt,
    kFloatArray,
    kDoubleArray,
    kText,
    kBlob
  };
  
  // for serializing
  struct BinaryHeader
  {
    unsigned int type : 4;
    unsigned int size : 28;
  };
  
  static constexpr size_t kStructSizeInBytes{64};
  static constexpr size_t kLocalDataBytes = kStructSizeInBytes - 16;

private:
  
  // data
  
  uint8_t* _dataPtr{_localData};
  uint32_t _type{kUndefined};
  uint32_t _sizeInBytes{0};
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
  Value (Value&& other) noexcept;
  Value& operator=(Value&& other) noexcept;
  ~Value();
  
  // Constructors with fixed-size data.
  
  Value();
  Value(float v);
  Value(double v);
  Value(bool v);
  Value(int v);
  
  // Constructors with variable-size data.

  Value(std::initializer_list<float> values);
  Value(const std::vector<float>& values);
  Value(const char* v);
  explicit Value(const ml::Text& v);
  explicit Value(const ml::Blob& v);
  Value(std::vector<uint8_t> dataVec); // alternate Blob
  Value(const BinaryHeader& header, const uint8_t* readPtr); // for reading binary data
  
  // getters for fixed-size data
  
  float getFloatValue() const;
  double getDoubleValue() const;
  bool getBoolValue() const;
  int getIntValue() const;

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
  uint32_t getType() const;
  uint32_t size() const;
  uint8_t* data() const;
  
  // constructor and getter for fixed-size float arrays
  
  template<size_t N>
  Value(std::array<float, N> values)
  {
    _type = kFloatArray;
    auto listSize = N;
    _sizeInBytes = listSize*sizeof(float);
    
    if(_sizeInBytes <= kLocalDataBytes)
    {
      // store locally
      _dataPtr = _localData;
      memcpy(_dataPtr, values.data(), _sizeInBytes);
    }
    else
    {
      // allocate heap
      _dataPtr = (uint8_t*)malloc(_sizeInBytes);
      if(_dataPtr)
      {
        memcpy(_dataPtr, values.data(), _sizeInBytes);
      }
      else
      {
        _dataPtr = _localData;
        _sizeInBytes = 0;
        _type = kUndefined;
      }
    }
  }
  
  template<size_t N>
  std::array<float, N> getFloatArray() const
  {
    std::array<float, N> r;
    if(_type == kFloatArray)
    {
      memcpy(r.data(), _dataPtr, _sizeInBytes);
    }
    return r;
  }
};

std::ostream& operator<<(std::ostream& out, const ml::Value& r);

// serialization

// return size of the binary representation of the Value (incl. type and size)
size_t getBinarySize(const Value& v);

// handy anything-converters to and from small POD types.

template<typename T>
inline Value valueFromPODType(T obj)
{
  static_assert(sizeof(T) <= Value::kLocalDataBytes);
  return Value(Blob(reinterpret_cast<const uint8_t*>(&obj), sizeof(T)));
}

template<typename T>
inline T valueToPODType(Value val)
{
  static_assert(sizeof(T) <= Value::kLocalDataBytes);
  T r;
  auto blob = val.getBlobValue();
  memcpy(&r, blob.data, blob.size);
  return r;
}

namespace ValueUtils
{
  // write the binary representation of the Value and increment the write pointer.
  void writeBinaryRepresentation(const Value& v, uint8_t*& writePtr);

  // read the binary representation of the Value and increment the read pointer.
  Value readBinaryRepresentation(const uint8_t*& readPtr);
}

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
