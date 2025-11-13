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
  type_ = newType;
  if (bytes == sizeInBytes_)
  {
    // same as existing size, copy to wherever data is currently stored
    memcpy(dataPtr_, pSrc, bytes);
  }
  else
  {
    if (dataPtr_ != localData_) free(dataPtr_);

    if (bytes <= kLocalDataBytes)
    {
      dataPtr_ = localData_;
      sizeInBytes_ = static_cast<uint32_t>(bytes);
      memcpy(dataPtr_, pSrc, sizeInBytes_);
    }
    else
    {
      dataPtr_ = (uint8_t*)malloc(bytes);
      if (dataPtr_)
      {
        sizeInBytes_ = static_cast<uint32_t>(bytes);
        memcpy(dataPtr_, pSrc, sizeInBytes_);
      }
      else
      {
        dataPtr_ = localData_;
        sizeInBytes_ = 0;
        type_ = kUndefined;
      }
    }
  }
}

void Value::copyOrMove(Type newType, uint8_t* pSrc, size_t bytes)
{
  type_ = newType;
  if (!isStoredLocally()) free(dataPtr_);
  if (bytes <= kLocalDataBytes)
  {
    sizeInBytes_ = static_cast<uint32_t>(bytes);
    dataPtr_ = localData_;
    memcpy(dataPtr_, pSrc, sizeInBytes_);
  }
  else
  {
    dataPtr_ = pSrc;
    sizeInBytes_ = static_cast<uint32_t>(bytes);
  }
}

// copy and assign constructors and destructor and movers (rule of five stuff)

Value::Value(const Value& other)
{
  copyOrAllocate(other.type_, other.dataPtr_, other.sizeInBytes_);
}

Value& Value::operator=(const Value& other)
{
  if (this == &other) return *this;
  copyOrAllocate(other.type_, other.dataPtr_, other.sizeInBytes_);
  return *this;
}

Value::Value(Value&& other) noexcept
{
  copyOrMove(other.type_, other.dataPtr_, other.sizeInBytes_);

  // Reset other to a valid empty state
  other.dataPtr_ = other.localData_;
  other.type_ = kUndefined;
  other.sizeInBytes_ = 0;
}

Value& Value::operator=(Value&& other) noexcept
{
  if (this == &other) return *this;
  copyOrMove(other.type_, other.dataPtr_, other.sizeInBytes_);

  // Reset other to a valid empty state
  other.dataPtr_ = other.localData_;
  other.type_ = kUndefined;
  other.sizeInBytes_ = 0;

  return *this;
}

Value::~Value()
{
  if (!isStoredLocally())
  {
    free(dataPtr_);
  }
}

// Constructors with fixed-size data.

Value::Value() : type_(kUndefined), sizeInBytes_(0), dataPtr_(localData_) {}

Value::Value(float v) : type_(kFloat), sizeInBytes_(sizeof(float)), dataPtr_(localData_)
{
  float* pVal = reinterpret_cast<float*>(localData_);
  pVal[0] = v;
}

Value::Value(double v) : type_(kFloat), sizeInBytes_(sizeof(float)), dataPtr_(localData_)
{
  float* pVal = reinterpret_cast<float*>(localData_);
  pVal[0] = static_cast<float>(v);
}

Value::Value(int v) : type_(kInt), sizeInBytes_(sizeof(int)), dataPtr_(localData_)
{
  auto pVal = reinterpret_cast<int*>(localData_);
  pVal[0] = v;
}

// Constructors with variable-size data.

Value::Value(std::initializer_list<float> values)
{
  const float* pFloatSrc = &*values.begin();
  auto pSrc = reinterpret_cast<const uint8_t*>(pFloatSrc);
  copyOrAllocate(kFloatArray, pSrc, values.size() * sizeof(float));
}

Value::Value(const std::vector<float>& values)
{
  const float* pFloatSrc = values.data();
  auto pSrc = reinterpret_cast<const uint8_t*>(pFloatSrc);
  copyOrAllocate(kFloatArray, pSrc, values.size() * sizeof(float));
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

Value::Value(const uint8_t* data, size_t size) { copyOrAllocate(kBlob, data, size); }

// Getters for scalar data.

float Value::getFloatValue() const
{
  switch (type_)
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
  switch (type_)
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
  switch (type_)
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
  return (type_ == kFloatArray) ? reinterpret_cast<float*>(dataPtr_) : nullptr;
}

size_t Value::getFloatArraySize() const
{
  return (type_ == kFloatArray) ? sizeInBytes_ / sizeof(float) : 0;
}

std::vector<float> Value::getFloatVector() const
{
  if (type_ != kFloatArray) return std::vector<float>();
  auto floatData = reinterpret_cast<float*>(dataPtr_);
  return std::vector<float>(floatData, floatData + sizeInBytes_ / sizeof(float));
}

TextFragment Value::getTextValue() const
{
  if (type_ != kText) return TextFragment();
  return TextFragment(reinterpret_cast<char*>(dataPtr_), sizeInBytes_);
}

// public utils

bool Value::isStoredLocally() const { return (dataPtr_ == localData_); }

// distinct from the bool value type, used to support code like if(doThingWithReturnValue())
Value::operator bool() const { return (type_ != kUndefined); }

bool Value::operator==(const Value& b) const
{
  if (type_ != b.type_) return false;
  if (sizeInBytes_ != b.sizeInBytes_) return false;
  return !std::memcmp(dataPtr_, b.dataPtr_, sizeInBytes_);
}

bool Value::operator!=(const Value& b) const { return !operator==(b); }

Value::Type Value::getType() const { return type_; }

const uint8_t* Value::data() const { return dataPtr_; }

uint32_t Value::size() const { return sizeInBytes_; }

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
      if (size > 8)
      {
        out << "[float array, size " << r.getFloatArraySize() << "]";
      }
      else if (size > 0)
      {
        out << "[";
        for (int i = 0; i < size - 1; ++i)
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
  type_ = static_cast<Value::Type>(type);
  sizeInBytes_ = sizeInBytes;

  if (sizeInBytes_ <= kLocalDataBytes)
  {
    dataPtr_ = localData_;
    memcpy(dataPtr_, dataPtr, sizeInBytes_);
  }
  else
  {
    dataPtr_ = (uint8_t*)malloc(sizeInBytes_);
    if (dataPtr_)
    {
      memcpy(dataPtr_, dataPtr, sizeInBytes_);
    }
    else
    {
      dataPtr_ = localData_;
      sizeInBytes_ = 0;
      type_ = kUndefined;
    }
  }
}

}  // namespace ml
