// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLValue.h"

#include "MLTextUtils.h"

namespace ml
{
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
  // TEMP just copy the bytes!!
  
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
    case kUnsignedLongValue:
      mUnsignedLongVal = other.getUnsignedLongValue();
      break;
  }
}

Value& Value::operator=(const Value& other)
{
  
  // TODO everything is in data, no switch, just copy bytes
  
  
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
    case kUnsignedLongValue:
      mUnsignedLongVal = other.getUnsignedLongValue();
      break;
  }

  return *this;
}

Value::Value(float v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(int v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(bool v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(unsigned long v) : mType(kUnsignedLongValue) { mUnsignedLongVal = static_cast<uint32_t>(v); }

// truncate to unsigned long for now. 
Value::Value(unsigned long long v) : mType(kUnsignedLongValue) { mUnsignedLongVal = static_cast<uint32_t>(v); }

Value::Value(uint32_t v) : mType(kUnsignedLongValue) { mUnsignedLongVal = v; }

Value::Value(long v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(double v) : mType(kFloatValue) { mFloatVal = v; }

Value::Value(const ml::Text& t) : mType(kTextValue) { mTextVal = t; }

Value::Value(const char* t) : mType(kTextValue) { mTextVal = ml::Text(t); }

Value::Value(const void* pData, size_t n) : mType(kBlobValue)
{
  copyBlob(pData, n);
}

Value::Value(const std::vector<uint8_t>& dataVec) : mType(kBlobValue)
{
  copyBlob(dataVec.data(), dataVec.size());
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

void Value::setValue(const Value& v) { *this = v; }

bool Value::operator==(const Value& b) const
{
  // TODO just check type, size, bytes
  
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
      case kUnsignedLongValue:
        r = (getUnsignedLongValue() == b.getUnsignedLongValue());
        break;
      case kBlobValue:
        // compare blobs by value
        r = !std::memcmp(getBlobData(), b.getBlobData(), getBlobSize());
        break;
    }
  }
  return r;
}

bool Value::operator!=(const Value& b) const { return !operator==(b); }

#pragma mark Value utilities

std::string getTypeDebugStr(const Value& r) {
  std::string out;
  switch (r.getType())
  {
    default:
    case Value::kUndefinedValue:
      out = "(?)";
      break;
    case Value::kFloatValue:
      out = "(F)";
      break;
    case Value::kTextValue:
      out = "(T)";
      break;
    case Value::kUnsignedLongValue:
      out = "(UL)";
      break;
    case Value::kBlobValue:
      out = "(B)";
      break;
  }
  return out;
}

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
    case Value::kUnsignedLongValue:
      out << r.getUnsignedLongValue();
      break;
    case Value::kBlobValue:
      out << "[blob, " << r.getBlobSize() << " bytes]";
      break;
  }
  return out;
}

}  // namespace ml
