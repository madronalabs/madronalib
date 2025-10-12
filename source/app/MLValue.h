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

// Value: a small unit of typed data designed for being constructed on the stack and
// transferred in messages. If the value to be constructed is below a certain size,
// no heap will be allocated. Copying a new value into one of the same size must not
// reallocate. Together these guarantees make Value useful for creating DSP applications
// that don't allocate memory when processing signals.

namespace ml
{

class Value
{
public:
  
  enum Type
  {
    kUndefined = 0,
    kFloat,
    kDouble,
    kInt,
    kFloatArray,
    kText,
    kBlob,
    kNumTypes
  };
  
  static constexpr size_t kMaxDataSizeBits{28};
  static constexpr size_t kMaxDataBytes = 1 << (kMaxDataSizeBits - 1);
  static constexpr size_t getLocalDataMaxBytes() { return kLocalDataBytes; }
  static constexpr size_t getHeaderBytes() { return kHeaderBytes; }
  
  // copy and assign constructors and destructor
  
  Value(const Value& other);
  Value& operator=(const Value& other);
  Value (Value&& other) noexcept;
  Value& operator=(Value&& other) noexcept;
  ~Value();
  
  // Constructors with fixed-size data.
  
  Value();
  Value(float v);
  Value(int v);
  
  // mostly we deal with floats, so set this explicit to avoid silent double->float conversions.
  explicit Value(double v);
  
  // fixed-size float arrays
  template<size_t N>
  explicit Value(std::array<float, N> values)
  {
    _type = kFloatArray;
    auto listSize = N;
    _sizeInBytes = static_cast<uint32_t>(listSize*sizeof(float));
    
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
  
  // Constructors with variable-size data.
  
  Value(std::initializer_list<float> values);
  Value(const std::vector<float>& values);
  explicit Value(const ml::Text& v);
  Value(const char* v);
  explicit Value(const uint8_t* data, size_t size);

  // getters for fixed-size data
  
  float getFloatValue() const;
  double getDoubleValue() const;
  bool getBoolValue() const;
  int getIntValue() const;
  
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
  
  // getters for variable-size data.
  
  float* getFloatArrayPtr() const;
  size_t getFloatArraySize() const;
  std::vector<float> getFloatVector() const;
  double* getDoubleArrayPtr() const;
  size_t getDoubleArraySize() const;
  std::vector<double> getDoubleVector() const;
  ml::TextFragment getTextValue() const;
  
  // public utils
  
  bool isStoredLocally() const;
  explicit operator bool() const;
  bool operator==(const Value& b) const;
  bool operator!=(const Value& b) const;
  Type getType() const;
  const uint8_t* data() const;
  uint32_t size() const;
  
  // can the Value be converted to the output Type without precision loss?
  bool canConvertTo(Type targetType) const;
  
private:
  
  // data
  
  // kHeaderBytes should be the size of everything except the local data. This is verified in valueTest.cpp.
  static constexpr size_t kStructSizeInBytes{64};
  static constexpr size_t kHeaderBytes{16};
  static constexpr size_t kLocalDataBytes = kStructSizeInBytes - kHeaderBytes;
  
  uint8_t* _dataPtr{_localData};
  Type _type{kUndefined};
  uint32_t _sizeInBytes{0};
  uint8_t _localData[kLocalDataBytes];
  
  // utilities
  void copyOrAllocate(Type type, const uint8_t* pSrc, size_t bytes);
  void copyOrMove(Type newType, uint8_t* pSrc, size_t bytes);
  
  template<typename T> T toFixedSizeType() const
  {
    T* scalarTypePtr = reinterpret_cast<T*>(_dataPtr);
    return *scalarTypePtr;
  }
  
  // private constructor for deserialization
  Value(unsigned int type, unsigned int sizeInBytes, const uint8_t* dataPtr);
  
  // friend in MLSerialization
  friend Value readBinaryToValue(const uint8_t*& readPtr);
};

std::ostream& operator<<(std::ostream& out, const ml::Value& r);

// Handy anything-converters to and from small POD types. These never allocate.
// There is no runtime type checking, so use carefully!

template<typename T>
inline Value smallTypeToValue(T obj)
{
  // create local Blob
  static_assert(sizeof(T) <= Value::getLocalDataMaxBytes());
  return Value(reinterpret_cast<const uint8_t*>(&obj), sizeof(T));
}

template<typename T>
inline T valueToSmallType(Value val)
{
  static_assert(sizeof(T) <= Value::getLocalDataMaxBytes());
  T r;
  memcpy(&r, val.data(), val.size());
  return r;
}

// NamedValue for initializer lists

struct NamedValue
{
  ml::Path name;
  Value value;
  
  template<typename T>
  NamedValue(ml::Path np, T nv) : name(np), value(Value(nv)) {}
  
  NamedValue(ml::Path np, std::initializer_list<float> list) : name(np), value(Value(list)) {}
};

// Define a type for initializing a new object with a list of Values.
using WithValues = const std::initializer_list< NamedValue >;

}  // namespace ml

