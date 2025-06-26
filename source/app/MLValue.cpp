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

void Value::copyOrMove(uint32_t newType, uint8_t* pSrc, size_t bytes)
{
  _type = newType;
  if(bytes <= kLocalDataBytes)
  {
    _sizeInBytes = static_cast<uint32_t>(bytes);
    _dataPtr = _localData;
    memcpy(_dataPtr, pSrc, _sizeInBytes);
  }
  else
  {
    if(!isStoredLocally()) free(_dataPtr);
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

// fixed-size getters cast simple types to whatever simple type is requested.
// other types we can't convert return a default value.

float Value::getFloatValue() const
{
  float r = 0.f;
  switch(_type)
  {
    case kFloat:
      r = toFixedSizeType<float>();
      break;
    case kDouble:
      r = toFixedSizeType<double>();
      break;
    case kBool:
      r = toFixedSizeType<bool>();
      break;
    case kInt:
      r = toFixedSizeType<int>();
      break;
    default:
      break;
  }
  return r;
}

double Value::getDoubleValue() const
{
  double r = 0.;
  switch(_type)
  {
    case kFloat:
      r = toFixedSizeType<float>();
      break;
    case kDouble:
      r = toFixedSizeType<double>();
      break;
    case kBool:
      r = toFixedSizeType<bool>();
      break;
    case kInt:
      r = toFixedSizeType<int>();
      break;
    default:
      break;
  }
  return r;
}

bool Value::getBoolValue() const
{
  bool r = 0;
  switch(_type)
  {
    case kFloat:
      r = toFixedSizeType<float>();
      break;
    case kDouble:
      r = toFixedSizeType<double>();
      break;
    case kBool:
      r = toFixedSizeType<bool>();
      break;
    case kInt:
      r = toFixedSizeType<int>();
      break;
    default:
      break;
  }
  return r;
}

int Value::getIntValue() const
{
  int r = 0.f;
  switch(_type)
  {
    case kFloat:
      r = toFixedSizeType<float>();
      break;
    case kDouble:
      r = toFixedSizeType<double>();
      break;
    case kBool:
      r = toFixedSizeType<bool>();
      break;
    case kInt:
      r = toFixedSizeType<int>();
      break;
    default:
      break;
  }
  return r;
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


std::string toHex(int value, int width) {
  const char* digits = "0123456789abcdef";
  std::string hex;

  if (value == 0) {
    hex = "0";
  } else {
    int temp = value;
    while (temp > 0) {
      hex = digits[temp % 16] + hex;
      temp /= 16;
    }
  }

  // Pad with zeros
  while (hex.length() < static_cast<size_t>(width)) {
    hex = "0" + hex;
  }

  return hex;
}


void Blob::dump() const
{
  auto blobData = data;
  if (!blobData)
  {
    std::cout << "empty blob! \n";
    return;
  }

  auto blobSize = size;
  const int bytesPerRow = 16;

  for (size_t i = 0; i < blobSize; i += bytesPerRow) {
    // Print offset
    std::cout << toHex(static_cast<int>(i), 8) << "  ";

    // Print hex values
    for (int j = 0; j < bytesPerRow; ++j) {
      if (i + j < blobSize) {
        std::cout << toHex(blobData[i + j], 2) << " ";
      } else {
        std::cout << "   "; // 3 spaces for missing bytes
      }

      // Add extra space after 8 bytes for readability
      if (j == 7) {
        std::cout << " ";
      }
    }

    std::cout << " |";

    // Print ASCII representation
    for (int j = 0; j < bytesPerRow && i + j < blobSize; ++j) {
      uint8_t byte = blobData[i + j];
      if (std::isprint(byte) && byte >= 32 && byte <= 126) {
        std::cout << static_cast<char>(byte);
      } else {
        std::cout << ".";
      }
    }

    std::cout << "|" << std::endl;
  }
}


}  // namespace ml


