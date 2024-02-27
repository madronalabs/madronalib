// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLValue.h"

#include "MLTextUtils.h"

namespace ml
{
const Matrix Value::nullMatrix{};

Value::Value() : mType(kUndefinedValue), mFloatVal(0) {}

void Value::copyBlob(const void* inputData, size_t size)
{
  // if we have external data, free it
  if(pBlobData != _localBlobData)
  {
    free(pBlobData);
    pBlobData = _localBlobData;
  }

  if (size <= kLocalDataBytes)
  {
    auto pCharData = (uint8_t*)inputData;
    std::copy(pCharData, pCharData + size, _localBlobData);
    _blobSizeInBytes = size;
  }
  else
  {
    pBlobData = (uint8_t*)malloc(size);
    if(pBlobData)
    {
      auto pCharData = (uint8_t*)(inputData);
      std::copy(pCharData, pCharData + size, pBlobData);
      _blobSizeInBytes = size;
    }
    else
    {
      // TODO throw?
      _blobSizeInBytes = 0;
    }
  }
}

Value::Value(const Value& other) : mType(other.getType()), mFloatVal(0)
{
  switch (mType)
  {
    case kUndefinedValue:
      break;
    case kFloatValue:
      mFloatVal = other.getFloatValue();
      break;
    case kTextValue:
      mTextVal = other.getTextValue();
      break;
    case kBlobValue:
      copyBlob(other.pBlobData, other._blobSizeInBytes);
      break;
    case kMatrixValue:
      mMatrixVal = other.getMatrixValue();
      break;
    case kUnsignedLongValue:
      mUnsignedLongVal = other.getUnsignedLongValue();
      break;
    case kIntervalValue:
      mIntervalVal = other.getIntervalValue();
      break;
  }
}

Value& Value::operator=(const Value& other)
{
  mType = other.getType();
  switch (mType)
  {
    case kUndefinedValue:
      break;
    case kFloatValue:
      mFloatVal = other.getFloatValue();
      break;
    case kTextValue:
      mTextVal = other.getTextValue();
      break;
    case kBlobValue:
      copyBlob(other.pBlobData, other._blobSizeInBytes);
      break;
    case kMatrixValue:
      // Matrix handles copy-in-place when possible
      mMatrixVal = other.getMatrixValue();
      break;
    case kUnsignedLongValue:
      mUnsignedLongVal = other.getUnsignedLongValue();
      break;
    case kIntervalValue:
      mIntervalVal = other.getIntervalValue();
      break;
  }

  return *this;
}

Value::Value(float v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(int v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(bool v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(unsigned long v) : mType(kUnsignedLongValue) { mUnsignedLongVal = v; }

// truncate to unsigned long for now. 
Value::Value(unsigned long long v) : mType(kUnsignedLongValue) { mUnsignedLongVal = static_cast<unsigned long>(v); }

Value::Value(uint32_t v) : mType(kUnsignedLongValue) { mUnsignedLongVal = v; }

Value::Value(long v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(double v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(const ml::Text& t) : mType(kTextValue) { mTextVal = t; }

Value::Value(const char* t) : mType(kTextValue) { mTextVal = ml::Text(t); }

Value::Value(const ml::Matrix& s) : mType(kMatrixValue) { mMatrixVal = s; }

Value::Value(Interval i) : mType(kIntervalValue) { mIntervalVal = i; }

Value::Value(const void* pData, size_t n) : mType(kBlobValue)
{
  copyBlob(pData, n);
}

Value::~Value()
{
  // if we have external data, free it
  if(pBlobData != _localBlobData)
  {
    free(pBlobData);
  }
}

void Value::setValue(const float& v)
{
  mType = kFloatValue;
  mFloatVal = v;
}

void Value::setValue(const int& v)
{
  mType = kFloatValue;
  mFloatVal = v;
}

void Value::setValue(const bool& v)
{
  mType = kFloatValue;
  mFloatVal = v;
}

void Value::setValue(const uint32_t& v)
{
  mType = kUnsignedLongValue;
  mUnsignedLongVal = v;
}

void Value::setValue(const long& v)
{
  mType = kFloatValue;
  mFloatVal = v;
}

void Value::setValue(const double& v)
{
  mType = kFloatValue;
  mFloatVal = v;
}

void Value::setValue(const ml::Text& v)
{
  mType = kTextValue;
  mTextVal = v;
}

void Value::setValue(const char* const v)
{
  mType = kTextValue;
  mTextVal = v;
}

void Value::setValue(const Matrix& v)
{
  mType = kMatrixValue;
  mMatrixVal = v;
}

void Value::setValue(const Interval v)
{
  mType = kIntervalValue;
  mIntervalVal = v;
}

void Value::setValue(const Value& v) { *this = v; }

bool Value::operator==(const Value& b) const
{
  bool r = false;
  if (mType == b.getType())
  {
    switch (mType)
    {
      case kUndefinedValue:
        r = true;
        break;
      case kFloatValue:
        r = (getFloatValue() == b.getFloatValue());
        break;
      case kTextValue:
        r = (getTextValue() == b.getTextValue());
        break;
      case kMatrixValue:
        r = (getMatrixValue() == b.getMatrixValue());
        break;
      case kUnsignedLongValue:
        r = (getUnsignedLongValue() == b.getUnsignedLongValue());
        break;
      case kBlobValue:
        r = false;
        break;
      case kIntervalValue:
        r = (getIntervalValue() == b.getIntervalValue());
        break;
    }
  }
  return r;
}

bool Value::operator!=(const Value& b) const { return !operator==(b); }

#pragma mark Value utilities

std::ostream& operator<<(std::ostream& out, const Value& r)
{
  switch (r.getType())
  {
    default:
    case Value::kUndefinedValue:
      out << "[undefined]";
      break;
    case Value::kFloatValue:
      out << r.getFloatValue();
      break;
    case Value::kTextValue:
      out << r.getTextValue();
      break;
    case Value::kMatrixValue:
      out << r.getMatrixValue();
      break;
    case Value::kUnsignedLongValue:
      out << r.getUnsignedLongValue();
      break;
    case Value::kBlobValue:
      out << "[blob]";
      break;
    case Value::kIntervalValue:
      out << r.getIntervalValue();
      break;
  }
  return out;
}

}  // namespace ml
