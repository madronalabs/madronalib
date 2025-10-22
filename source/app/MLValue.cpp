// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLValue.h"

#include "MLTextUtils.h"

namespace ml
{

// private utilities

void Value::copyOrAllocate(Type newType, const uint8_t* pSrc, size_t bytes)
{
  _type = newType;
  if(bytes == _sizeInBytes)
  {
    // same as existing size, copy to wherever data is currently stored
    memcpy(_dataPtr, pSrc, bytes);
  }
  else
  {
    if(_dataPtr != _localData) free(_dataPtr);
    
    if(bytes <= kLocalDataBytes)
    {
      _dataPtr = _localData;
      _sizeInBytes = static_cast<uint32_t>(bytes);
      memcpy(_dataPtr, pSrc, _sizeInBytes);
    }
    else
    {
      _dataPtr = (uint8_t*)malloc(bytes);
      if(_dataPtr)
      {
        _sizeInBytes = static_cast<uint32_t>(bytes);
        memcpy(_dataPtr, pSrc, _sizeInBytes);
      }
      else
      {
        _dataPtr = _localData;
        _sizeInBytes = 0;
        _type = kUndefined;
      }
    }
  }
}

void Value::copyOrMove(Type newType, uint8_t* pSrc, size_t bytes)
{
  _type = newType;
  if(!isStoredLocally()) free(_dataPtr);
  if(bytes <= kLocalDataBytes)
  {
    _sizeInBytes = static_cast<uint32_t>(bytes);
    _dataPtr = _localData;
    memcpy(_dataPtr, pSrc, _sizeInBytes);
  }
  else
  {
    _dataPtr = pSrc;
    _sizeInBytes = static_cast<uint32_t>(bytes);
  }
}

// copy and assign constructors and destructor and movers (rule of five stuff)

Value::Value(const Value& other)
{
  copyOrAllocate(other._type, other._dataPtr, other._sizeInBytes);
}

Value& Value::operator=(const Value& other)
{
  if(this == &other) return *this;
  copyOrAllocate(other._type, other._dataPtr, other._sizeInBytes);
  return *this;
}

Value::Value(Value&& other) noexcept
{
  copyOrMove(other._type, other._dataPtr, other._sizeInBytes);

  // Reset other to a valid empty state
  other._dataPtr = other._localData;
  other._type = kUndefined;
  other._sizeInBytes = 0;
}

Value& Value::operator=(Value&& other) noexcept
{
  if(this == &other) return *this;
  copyOrMove(other._type, other._dataPtr, other._sizeInBytes);
  
  // Reset other to a valid empty state
  other._dataPtr = other._localData;
  other._type = kUndefined;
  other._sizeInBytes = 0;
  
  return *this;
}

Value::~Value()
{
  if(!isStoredLocally())
  {
    free(_dataPtr);
  }
}

// Constructors with fixed-size data.

Value::Value() : _type(kUndefined), _sizeInBytes(0), _dataPtr(_localData) {}

Value::Value(float v) : _type(kFloat), _sizeInBytes(sizeof(float)), _dataPtr(_localData)
{
  float* pVal = reinterpret_cast<float*>(_localData);
  pVal[0] = v;
}

Value::Value(double v) : _type(kFloat), _sizeInBytes(sizeof(float)), _dataPtr(_localData)
{
  float* pVal = reinterpret_cast<float*>(_localData);
  pVal[0] = static_cast< float >(v);
}

Value::Value(int v) : _type(kInt), _sizeInBytes(sizeof(int)), _dataPtr(_localData)
{
  int* pVal = reinterpret_cast<int*>(_localData);
  pVal[0] = v;
}

// Constructors with variable-size data.

Value::Value(std::initializer_list<float> values)
{
  const float* pFloatSrc = &*values.begin();
  auto pSrc = reinterpret_cast<const uint8_t*>(pFloatSrc);
  copyOrAllocate(kFloatArray, pSrc, values.size()*sizeof(float));
}

Value::Value(const std::vector<float>& values)
{
  const float* pFloatSrc = values.data();
  auto pSrc = reinterpret_cast<const uint8_t*>(pFloatSrc);
  copyOrAllocate(kFloatArray, pSrc, values.size()*sizeof(float));
}

Value::Value(const ml::Text& t)
{
  auto pSrc = reinterpret_cast<const uint8_t*>(t.getText());
  copyOrAllocate(kText, pSrc, t.lengthInBytes());
}

Value::Value(const char* v)
{
  auto pSrc = reinterpret_cast<const uint8_t*>(v);
  size_t len = strlen(v);
  copyOrAllocate(kText, pSrc, len);
}

Value::Value(const uint8_t* data, size_t size)
{
  copyOrAllocate(kBlob, data, size);
}


// Getters for scalar data. 

float Value::getFloatValue() const
{
  switch(_type)
  {
    case kFloat:
      return toFixedSizeType<float>();
      break;
    case kInt:
      return static_cast<float>(toFixedSizeType<int>());
      break;
      
    // not convertible
    default:
      return 0.f;
      break;
  }
}

int Value::getIntValue() const
{
  switch(_type)
  {
    case kFloat:
      return static_cast<int>(toFixedSizeType<float>());
      break;
    case kInt:
      return toFixedSizeType<int>();
      break;
      
    // not convertible
    default:
      return 0;
      break;
  }
}

bool Value::getBoolValue() const
{
  switch(_type)
  {
    case kFloat:
      return (toFixedSizeType<float>() != 0.0f);
      break;
    case kInt:
      return toFixedSizeType<int>() != 0;
      break;
      
    // not convertible
    default:
      return false;
      break;
  }
}

// variable-size getters

float* Value::getFloatArrayPtr() const
{
  return (_type == kFloatArray) ? reinterpret_cast<float*>(_dataPtr) : nullptr;
}

size_t Value::getFloatArraySize() const
{
  return (_type == kFloatArray) ? _sizeInBytes/sizeof(float) : 0;
}

std::vector<float> Value::getFloatVector() const
{
  if(_type != kFloatArray) return std::vector<float>();
  auto floatData = reinterpret_cast<float*>(_dataPtr);
  return std::vector<float>(floatData, floatData + _sizeInBytes/sizeof(float));
}

TextFragment Value::getTextValue() const
{
  if(_type != kText) return TextFragment();
  return TextFragment(reinterpret_cast<char *>(_dataPtr), _sizeInBytes);
}

// public utils

bool Value::isStoredLocally() const
{
  return (_dataPtr == _localData);
}

// distinct from the bool value type, used to support code like if(doThingWithReturnValue())
Value::operator bool() const
{
  return (_type != kUndefined);
}

bool Value::operator==(const Value& b) const
{
  if(_type != b._type) return false;
  if(_sizeInBytes != b._sizeInBytes) return false;
  return !std::memcmp(_dataPtr, b._dataPtr, _sizeInBytes);
}

bool Value::operator!=(const Value& b) const
{
  return !operator==(b);
}

Value::Type Value::getType() const
{
  return _type;
}

const uint8_t* Value::data() const
{
  return _dataPtr;
}

uint32_t Value::size() const
{
  return _sizeInBytes;
}

std::ostream& operator<<(std::ostream& out, const Value& r)
{
  switch (r.getType())
  {
    default:
    case Value::kUndefined:
      out << "[undefined]";
      break;
    case Value::kFloat:
      out << r.getFloatValue();
      break;
    case Value::kInt:
      out << r.getIntValue();
      break;
    case Value::kFloatArray:
    {
      auto data = r.getFloatArrayPtr();
      auto size = r.getFloatArraySize();
      if(size > 8)
      {
        out << "[float array, size " << r.getFloatArraySize() << "]";
      }
      else if(size > 0)
      {
        out << "[";
        for(int i=0; i<size - 1; ++i)
        {
          out << data[i] << ", ";
        }
        out << data[size - 1];
        out << "]";
      }
      else
      {
        out << "[float array, SIZE 0!!!]\n";
      }
      break;
    }
    case Value::kText:
      out << r.getTextValue();
      break;
    case Value::kBlob:
      out << "[blob, " << r.size() << " bytes]";
      break;
  }
  return out;
}

Value::Value(unsigned int type, unsigned int sizeInBytes, const uint8_t* dataPtr)
{
  _type = static_cast<Value::Type>(type);
  _sizeInBytes = sizeInBytes;
  
  if(_sizeInBytes <= kLocalDataBytes)
  {
    _dataPtr = _localData;
    memcpy(_dataPtr, dataPtr, _sizeInBytes);
  }
  else
  {
    _dataPtr = (uint8_t*)malloc(_sizeInBytes);
    if(_dataPtr)
    {
      memcpy(_dataPtr, dataPtr, _sizeInBytes);
    }
    else
    {
      _dataPtr = _localData;
      _sizeInBytes = 0;
      _type = kUndefined;
    }
  }
}

}  // namespace ml


