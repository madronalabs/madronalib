// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
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

void Value::copyOrAllocate(uint32_t newType, const uint8_t* pSrc, size_t bytes)
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
      _sizeInBytes = bytes;
      memcpy(_dataPtr, pSrc, _sizeInBytes);
    }
    else
    {
      _dataPtr = (uint8_t*)malloc(bytes);
      if(_dataPtr)
      {
        _sizeInBytes = bytes;
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

void Value::copyOrMove(uint32_t newType, uint8_t* pSrc, size_t bytes)
{
  _type = newType;
  if(bytes <= kLocalDataBytes)
  {
    _sizeInBytes = bytes;
    _dataPtr = _localData;
    memcpy(_dataPtr, pSrc, _sizeInBytes);
  }
  else
  {
    if(!isStoredLocally()) free(_dataPtr);
    _dataPtr = pSrc;
    _sizeInBytes = bytes;
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
  other._dataPtr = other._localData;
}

Value& Value::operator=(Value&& other) noexcept
{
  if(this == &other) return *this;
  copyOrMove(other._type, other._dataPtr, other._sizeInBytes);
  other._dataPtr = other._localData;
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

static Value testValue();

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
  len = std::min(len, kMaxDataBytes);
  copyOrAllocate(kText, pSrc, len);
}

Value::Value(const ml::Blob& v)
{
  copyOrAllocate(kBlob, v.data, v.size);
}

Value::Value(const std::vector<uint8_t>& v)
{
  size_t len = v.size();
  len = std::min(len, kMaxDataBytes);
  copyOrAllocate(kBlob, v.data(), len);
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

int Value::getIntValue() const
{
  return (_type == kInt) ? toFixedSizeType<int>() : 0;
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
  return Blob(_dataPtr, _sizeInBytes);
}

std::vector<uint8_t> Value::getBlobVector() const
{
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

uint32_t Value::getType() const
{
  return _type;
}

uint32_t Value::size() const
{
  return _sizeInBytes;
}

uint8_t* Value::data() const
{
  return _dataPtr;
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
    case Value::kInt:
      out << r.getIntValue();
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

// return size of the binary representation of the Value (incl. type and size)
size_t getBinarySize(const Value& v)
{
  return v.size() + sizeof(Value::BinaryHeader);
}

Value::Value(const Value::BinaryHeader& header, const uint8_t* readPtr)
{
  _type = header.type;
  _sizeInBytes = header.size;
  
  if(_sizeInBytes <= kLocalDataBytes)
  {
    // store locally
    _dataPtr = _localData;
    memcpy(_dataPtr, readPtr, _sizeInBytes);
  }
  else
  {
    // allocate heap
    _dataPtr = (uint8_t*)malloc(_sizeInBytes);
    if(_dataPtr)
    {
      memcpy(_dataPtr, readPtr, _sizeInBytes);
    }
    else
    {
      _dataPtr = _localData;
      _sizeInBytes = 0;
      _type = kUndefined;
    }
  }
}

// write the binary representation of the Value and increment the destination pointer.
void Value::writeBinaryRepresentation(const Value& v, uint8_t*& writePtr)
{
  // header
  auto sizeInBytes = v.size();
  Value::BinaryHeader header{v.getType(), sizeInBytes};
  memcpy(writePtr, &header, sizeof(Value::BinaryHeader));
  writePtr += sizeof(Value::BinaryHeader);
  
  // data
  memcpy(writePtr, v.data(), sizeInBytes);
  writePtr += sizeInBytes;
}

Value Value::readBinaryRepresentation(const uint8_t*& readPtr)
{
  auto headerSize = sizeof(Value::BinaryHeader);
  
  // header
  Value::BinaryHeader header;
  memcpy(&header, readPtr, headerSize);
  
  const uint8_t* dataPtr = readPtr + headerSize;
  readPtr += header.size + headerSize;
  
  return Value(header, dataPtr);
}


}  // namespace ml


