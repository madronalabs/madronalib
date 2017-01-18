
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProperty.h"

#pragma mark MLProperty

const MLSignal MLProperty::nullSignal;

MLProperty::MLProperty() :
	mType(kUndefinedProperty),
	mFloatVal(0)
{
}

MLProperty::MLProperty(const MLProperty& other) :
	mType(other.getType()),
	mFloatVal(0)
{
	switch(mType)
	{
		case MLProperty::kFloatProperty:
			mFloatVal = other.getFloatValue();
			break;
		case MLProperty::kTextProperty:
			mTextVal = other.getTextValue();
			break;
		case MLProperty::kSignalProperty:
			mSignalVal = other.getSignalValue();
			break;
		default:
			break;
	}
}

MLProperty& MLProperty::operator= (const MLProperty& other)
{
	// copy data in place if possible
	mType = other.getType();
	switch(mType)
	{
		case MLProperty::kFloatProperty:
			mFloatVal = other.getFloatValue();
			break;
		case MLProperty::kTextProperty:
			mTextVal = other.getTextValue();
			break;
		case MLProperty::kSignalProperty:
			// MLSignal handles copy-in-place when possible
			mSignalVal = other.getSignalValue();
			break;
		default:
			break;
	}

	return *this;
}

MLProperty::MLProperty(float v) :
mType(kFloatProperty)
{
	mFloatVal = v;
}

MLProperty::MLProperty(int v) :
mType(kFloatProperty)
{
	mFloatVal = v;
}

MLProperty::MLProperty(long v) :
mType(kFloatProperty)
{
	mFloatVal = v;
}

MLProperty::MLProperty(double v) :
mType(kFloatProperty)
{
	mFloatVal = v;
}

MLProperty::MLProperty(const ml::Text& t) :
mType(kTextProperty)
{
	mTextVal = t;
}

MLProperty::MLProperty(const char* t) :
mType(kTextProperty)
{
	mTextVal = ml::Text(t);
}

MLProperty::MLProperty(const MLSignal& s) :
	mType(kSignalProperty)
{
	mSignalVal = s;
}

MLProperty::~MLProperty()
{
}

const float MLProperty::getFloatValue() const
{
	static const float nullFloat = 0.f;
	return (mType == kFloatProperty) ? mFloatVal : nullFloat;
}

const ml::Text MLProperty::getTextValue() const
{
	return (mType == kTextProperty) ? (mTextVal) : ml::Text();
}

const MLSignal& MLProperty::getSignalValue() const
{
	return (mType == kSignalProperty) ? (mSignalVal) : nullSignal;
}

void MLProperty::setValue(const float& v)
{
	mType = kFloatProperty;
	mFloatVal = v;
}

void MLProperty::setValue(const int& v)
{
	mType = kFloatProperty;
	mFloatVal = v;
}

void MLProperty::setValue(const long& v)
{
	mType = kFloatProperty;
	mFloatVal = v;
}

void MLProperty::setValue(const double& v)
{
	mType = kFloatProperty;
	mFloatVal = v;
}

void MLProperty::setValue(const ml::Text& v)
{
	mType = kTextProperty;
	mTextVal = v;
}

void MLProperty::setValue(const char* const v)
{
	mType = kTextProperty;
	mTextVal = v;
}

void MLProperty::setValue(const MLSignal& v)
{
	mType = kSignalProperty;
	mSignalVal = v;
}

void MLProperty::setValue(const MLProperty& v)
{
	*this = v;
}

bool MLProperty::operator== (const MLProperty& b) const
{
	bool r = false;
	if(mType == b.getType())
	{
		switch(mType)
		{
			case kUndefinedProperty:
				r = true;
				break;
			case kFloatProperty:
				r = (getFloatValue() == b.getFloatValue());
				break;
			case kTextProperty:
				r = (getTextValue() == b.getTextValue());
				break;
			case kSignalProperty:
				r = (getSignalValue() == b.getSignalValue());
				break;
		}
	}
	return r;
}

bool MLProperty::operator!= (const MLProperty& b) const
{
	return !operator==(b);
}

#pragma mark MLProperty utilities

std::ostream& operator<< (std::ostream& out, const MLProperty & r)
{
	switch(r.getType())
	{
		case MLProperty::kUndefinedProperty:
			out << "[undefined]";
			break;
		case MLProperty::kFloatProperty:
			out << r.getFloatValue();
			break;
		case MLProperty::kTextProperty:
			out << r.getTextValue();
			break;
		case MLProperty::kSignalProperty:
			out << r.getSignalValue();
            break;
	}
	return out;
}
