
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLValue.h"

using namespace ml;

const Matrix Value::nullSignal;

Value::Value() :
	mType(kUndefinedValue),
	mFloatVal(0)
{
}

Value::Value(const Value& other) :
	mType(other.getType()),
	mFloatVal(0)
{
	switch(mType)
	{
		case Value::kFloatValue:
			mFloatVal = other.getFloatValue();
			break;
		case Value::kTextValue:
			mTextVal = other.getTextValue();
			break;
		case Value::kMatrixValue:
			mMatrixVal = other.getMatrixValue();
			break;
		default:
			break;
	}
}

Value& Value::operator= (const Value& other)
{
  mType = other.getType();
  switch(mType)
  {
    case Value::kFloatValue:
      mFloatVal = other.getFloatValue();
      break;
    case Value::kTextValue:
      mTextVal = other.getTextValue();
      break;
    case Value::kMatrixValue:
      // Matrix handles copy-in-place when possible
      mMatrixVal = other.getMatrixValue();
      break;
    default:
      break;
  }

	return *this;
}

Value::Value(float v) :
mType(kFloatValue)
{
	mFloatVal = v;
}

Value::Value(int v) :
mType(kFloatValue)
{
	mFloatVal = v;
}

Value::Value(long v) :
mType(kFloatValue)
{
	mFloatVal = v;
}

Value::Value(double v) :
mType(kFloatValue)
{
	mFloatVal = v;
}

Value::Value(const ml::Text& t) :
mType(kTextValue)
{
	mTextVal = t;
}

Value::Value(const char* t) :
mType(kTextValue)
{
	mTextVal = ml::Text(t);
}

Value::Value(const ml::Matrix& s) :
	mType(kMatrixValue)
{
	mMatrixVal = s;
}

Value::~Value()
{
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

void Value::setValue(const Value& v)
{
	*this = v;
}

bool Value::operator== (const Value& b) const
{
	bool r = false;
	if(mType == b.getType())
	{
		switch(mType)
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
		}
	}
	return r;
}

bool Value::operator!= (const Value& b) const
{
	return !operator==(b);
}

#pragma mark Value utilities

std::ostream& operator<< (std::ostream& out, const Value & r)
{
	switch(r.getType())
	{
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
	}
	return out;
}
