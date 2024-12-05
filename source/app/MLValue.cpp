// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLValue.h"

#include "MLTextUtils.h"

namespace ml
{

// private utilities

bool Value::isStoredLocally() const
{
  return (_dataPtr == _localData);
}

void Value::copyLocalData(const Value& other)
{
  _type = other._type;
  _sizeInBytes = other._sizeInBytes;
  _dataPtr = _localData;
  memcpy(_localData, other._localData, _sizeInBytes);
}

// note _localData is unused when data is external. This is only a good tradeoff
// because we expect data to be local nearly all of the time.
void Value::reallocateAndCopy(const Value& other)
{
  free(_dataPtr);
  _dataPtr = (uint8_t*)malloc(other._sizeInBytes);
  if(_dataPtr)
  {
    std::copy(other._dataPtr, other._dataPtr + other._sizeInBytes, _dataPtr);
    _sizeInBytes = other._sizeInBytes;
    _type = other._type;
  }
  else
  {
    _dataPtr = _localData;
    _sizeInBytes = 0;
    _type = kUndefined;
  }
}

// copy and assign constructors and destructor and movers (rule of five stuff)

Value::Value(const Value& other)
{
  if(other.isStoredLocally())
  {
    copyLocalData(other);
  }
  else
  {
    reallocateAndCopy(other);
  }
}

Value& Value::operator=(const Value& other)
{
  if(this == &other) return *this;

  if(other.isStoredLocally())
  {
    copyLocalData(other);
  }
  else
  {
    reallocateAndCopy(other);
  }

  return *this;
}


Value::Value(Value&& other) noexcept
{
  if(other.isStoredLocally())
  {
    copyLocalData(other);
  }
  else
  {
    _type = other._type;
    _dataPtr = other._dataPtr;
    _sizeInBytes = other._sizeInBytes;
    other._dataPtr = other._localData;
  }
}

Value& Value::operator=(Value&& other) noexcept
{
  if(this != &other)
  {    
    if(other.isStoredLocally())
    {
      copyLocalData(other);
    }
    else
    {
      if(!isStoredLocally()) free(_dataPtr);
      _type = other._type;
      _dataPtr = other._dataPtr;
      _sizeInBytes = other._sizeInBytes;
      other._dataPtr = other._localData;
    }
  }
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

Value::Value(double v) : _type(kDouble), _sizeInBytes(sizeof(double)), _dataPtr(_localData)
{
  double* pVal = reinterpret_cast<double*>(_localData);
  pVal[0] = v;
}

Value::Value(bool v) : _type(kBool), _sizeInBytes(sizeof(bool)), _dataPtr(_localData)
{
  bool* pVal = reinterpret_cast<bool*>(_localData);
  pVal[0] = v;
}

// int -> uint32
Value::Value(int v) : _type(kUInt32), _sizeInBytes(sizeof(uint32_t)), _dataPtr(_localData)
{
  uint32_t* pVal = reinterpret_cast<uint32_t*>(_localData);
  pVal[0] = v;
}

/*
Value::Value(int32_t v) : _type(kInt32), _sizeInBytes(sizeof(int32_t)), _dataPtr(_localData)
{
  int32_t* pVal = reinterpret_cast<int32_t*>(_localData);
  pVal[0] = v;
}

Value::Value(int64_t v) : _type(kInt64), _sizeInBytes(sizeof(int64_t)), _dataPtr(_localData)
{
  int64_t* pVal = reinterpret_cast<int64_t*>(_localData);
  pVal[0] = v;
}

Value::Value(uint32_t v) : _type(kUInt32), _sizeInBytes(sizeof(uint32_t)), _dataPtr(_localData)
{
  uint32_t* pVal = reinterpret_cast<uint32_t*>(_localData);
  pVal[0] = v;
}

Value::Value(uint64_t v) : _type(kUInt64), _sizeInBytes(sizeof(uint64_t)), _dataPtr(_localData)
{
  uint64_t* pVal = reinterpret_cast<uint64_t*>(_localData);
  pVal[0] = v;
}
*/

// Constructors with variable-size data.
// if data size > kLocalDataBytes, these will allocate heap memory.

Value::Value(std::initializer_list<float> values)
{
  _type = kFloatArray;
  auto listSize = values.size();
  _sizeInBytes = listSize*sizeof(float);

  if(_sizeInBytes <= kLocalDataBytes)
  {
    // store locally
    _dataPtr = _localData;

    float* pDest = reinterpret_cast<float*>(_dataPtr);
    for(const float& value : values)
    {
      pDest[0] = value;
      pDest++;
    }
  }
  else
  {
    // allocate heap
    _dataPtr = (uint8_t*)malloc(_sizeInBytes);
    if(_dataPtr)
    {
      float* pDest = reinterpret_cast<float*>(_dataPtr);
      for(const float& value : values)
      {
        pDest[0] = value;
        pDest++;
      }
    }
    else
    {
      _dataPtr = _localData;
      _sizeInBytes = 0;
      _type = kUndefined;
    }
  }
}

Value::Value(const ml::Text& t)
{
  _type = kText;
  _sizeInBytes = t.lengthInBytes();

  if(_sizeInBytes <= kLocalDataBytes)
  {
    // store locally
    _dataPtr = _localData;
    memcpy(_dataPtr, t.getText(), _sizeInBytes);
  }
  else
  {
    // allocate heap
    _dataPtr = (uint8_t*)malloc(_sizeInBytes);
    if(_dataPtr)
    {
      memcpy(_dataPtr, t.getText(), _sizeInBytes);
    }
    else
    {
      _dataPtr = _localData;
      _sizeInBytes = 0;
      _type = kUndefined;
    }
  }
}

Value::Value(const char* v)
{
  _type = kText;
  _sizeInBytes = strlen(v);
  
  if(_sizeInBytes <= kLocalDataBytes)
  {
    // store locally
    _dataPtr = _localData;
    memcpy(_dataPtr, v, _sizeInBytes);
  }
  else
  {
    // allocate heap
    _dataPtr = (uint8_t*)malloc(_sizeInBytes);
    if(_dataPtr)
    {
      memcpy(_dataPtr, v, _sizeInBytes);
    }
    else
    {
      _dataPtr = _localData;
      _sizeInBytes = 0;
      _type = kUndefined;
    }
  }
}

Value::Value(const ml::Blob& v)
{
  _type = kBlob;
  _sizeInBytes = v.size;

  if(_sizeInBytes <= kLocalDataBytes)
  {
    // store locally
    _dataPtr = _localData;
    memcpy(_dataPtr, v.data, _sizeInBytes);
  }
  else
  {
    // allocate heap
    _dataPtr = (uint8_t*)malloc(_sizeInBytes);
    if(_dataPtr)
    {
      memcpy(_dataPtr, v.data, _sizeInBytes);
    }
    else
    {
      _dataPtr = _localData;
      _sizeInBytes = 0;
      _type = kUndefined;
    }
  }
}

Value::Value(std::vector<uint8_t> v)
{
  _type = kBlob;
  _sizeInBytes = v.size();
  
  if(_sizeInBytes <= kLocalDataBytes)
  {
    // store locally
    _dataPtr = _localData;
    memcpy(_dataPtr, v.data(), _sizeInBytes);
  }
  else
  {
    // allocate heap
    _dataPtr = (uint8_t*)malloc(_sizeInBytes);
    if(_dataPtr)
    {
      memcpy(_dataPtr, v.data(), _sizeInBytes);
    }
    else
    {
      _dataPtr = _localData;
      _sizeInBytes = 0;
      _type = kUndefined;
    }
  }
}


// fixed-size getters

float Value::getFloatValue() const
{
  return (_type == kFloat) ? toFixedSizeType<float>() : 0.0f;
}

double Value::getDoubleValue() const
{
  return (_type == kDouble) ? toFixedSizeType<double>() : 0.0;
}

bool Value::getBoolValue() const
{
  return (_type == kBool) ? toFixedSizeType<bool>() : false;
}

int32_t Value::getInt32Value() const
{
  return (_type == kInt32) ? toFixedSizeType<int32_t>() : 0;
}

int64_t Value::getInt64Value() const
{
  return (_type == kInt64) ? toFixedSizeType<int64_t>() : 0;
}

uint32_t Value::getUInt32Value() const
{
  return (_type == kUInt32) ? toFixedSizeType<uint32_t>() : 0;
}

uint64_t Value::getUInt64Value() const
{
  return (_type == kUInt64) ? toFixedSizeType<uint64_t>() : 0;
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

double* Value::getDoubleArrayPtr() const
{
  return (_type == kDoubleArray) ? reinterpret_cast<double*>(_dataPtr) : nullptr;
}

size_t Value::getDoubleArraySize() const
{
  return (_type == kDoubleArray) ? _sizeInBytes/sizeof(double) : 0;
}

std::vector<double> Value::getDoubleVector() const
{
  if(_type != kDoubleArray) return std::vector<double>();
  auto doubleData = reinterpret_cast<double*>(_dataPtr);
  return std::vector<double>(doubleData, doubleData + _sizeInBytes/sizeof(double));
}

TextFragment Value::getTextValue() const
{
  if(_type != kText) return TextFragment();
  return TextFragment(reinterpret_cast<char *>(_dataPtr), _sizeInBytes);
}

ml::Blob Value::getBlobValue() const
{
  if(_type != kBlob) return Blob();
  return Blob(_dataPtr, _sizeInBytes);
}

std::vector<uint8_t> Value::getBlobVector() const
{
  if(_type != kBlob) return std::vector<uint8_t>();
  return std::vector<uint8_t>(_dataPtr, _dataPtr + _sizeInBytes);
}




// public utils

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
    case Value::kDouble:
      out << r.getDoubleValue();
      break;
    case Value::kBool:
      out << r.getBoolValue();
      break;
    case Value::kInt32:
      out << r.getInt32Value();
      break;
    case Value::kInt64:
      out << r.getInt64Value();
      break;
    case Value::kUInt32:
      out << r.getUInt32Value();
      break;
    case Value::kUInt64:
      out << r.getUInt64Value();
      break;
    case Value::kFloatArray:
      out << "[float array, size " << r.getFloatArraySize() << "]";
      break;
    case Value::kDoubleArray:
      out << "[double array, size " << r.getDoubleArraySize() << "]";
      break;
    case Value::kText:
      out << r.getTextValue();
      break;
    case Value::kBlob:
      Blob b = r.getBlobValue();
      out << "[blob, " << b.size << " bytes]";
      break;
  }
  return out;
}

}  // namespace ml
