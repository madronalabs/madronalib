
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLModelParam.h"

static const std::string kNullModelParamString;
static const MLSignal kNullModelParamSignal;

// ----------------------------------------------------------------
// MLModelParam implementation


MLModelParam::MLModelParam() :
	mType(kUndefinedParam)
{
	mVal.mpStringVal = 0;
}

MLModelParam::MLModelParam(const MLModelParam& other) :
	mType(other.getType())
{ 
	switch(mType)
	{
		case MLModelParam::kFloatParam:
			mVal.mFloatVal = other.getFloatValue();
			break;
		case MLModelParam::kStringParam:
			mVal.mpStringVal = new std::string(other.getStringValue());
			break;
		case MLModelParam::kSignalParam:
			mVal.mpSignalVal = new MLSignal(other.getSignalValue());
			break;
		default:
			mVal.mpStringVal = 0;
			break;
	}
}

MLModelParam& MLModelParam::operator= (const MLModelParam& other)
{
	mType = other.getType();
	switch(mType)
	{
		case MLModelParam::kFloatParam:
			mVal.mFloatVal = other.getFloatValue();
			break;
		case MLModelParam::kStringParam:
			mVal.mpStringVal = new std::string(other.getStringValue());
			break;
		case MLModelParam::kSignalParam:
			mVal.mpSignalVal = new MLSignal(other.getSignalValue());
			break;
		default:
			mVal.mpStringVal = 0;
			break;
	}
	return *this;
}

MLModelParam::MLModelParam(float v) :
	mType(kFloatParam)
{
	mVal.mFloatVal = v;
}

MLModelParam::MLModelParam(const std::string& s) :
	mType(kStringParam)
{ 
	mVal.mpStringVal = new std::string(s); 
}

MLModelParam::MLModelParam(const MLSignal& s) :
	mType(kSignalParam)
{
	mVal.mpSignalVal = new MLSignal(s); 
}

MLModelParam::~MLModelParam() 
{
	switch(mType)
	{
		case kStringParam:
			delete mVal.mpStringVal;
			break;
		case kSignalParam:
			delete mVal.mpSignalVal;
			break;
		default:
			break;
	}
}

float MLModelParam::getFloatValue() const
{
	return mVal.mFloatVal;
}

const std::string& MLModelParam::getStringValue() const
{
	return *(mVal.mpStringVal);
}

const MLSignal& MLModelParam::getSignalValue() const
{
	return *(mVal.mpSignalVal);
}

void MLModelParam::setValue(float v)
{
	if(mType == kUndefinedParam) 
		mType = kFloatParam;
	if(mType == kFloatParam)
	{
		mVal.mFloatVal = v;
	}
	else 
	{
		debug() << "MLModelParam::setValue: type mismatch!\n";
	}
}

void MLModelParam::setValue(const std::string& v)
{
	if(mType == kUndefinedParam) 
		mType = kStringParam;
	if(mType == kStringParam)
	{
		delete mVal.mpStringVal;
		mVal.mpStringVal = new std::string(v);
	}
	else 
	{
		debug() << "MLModelParam::setValue: type mismatch!\n";
	}
}

void MLModelParam::setValue(const MLSignal& v)
{
	if(mType == kUndefinedParam) 
		mType = kSignalParam;
	if(mType == kSignalParam)
	{
		delete mVal.mpSignalVal;
		mVal.mpSignalVal = new MLSignal(v);
	}
	else 
	{
		debug() << "MLModelParam::setValue: type mismatch!\n";
	}
}

bool MLModelParam::operator== (const MLModelParam& b) const
{
	bool r = false;
	if(mType == b.getType())
	{
		switch(mType)
		{
			case kUndefinedParam:
				r = true;
				break;
			case kFloatParam:	
				r = (getFloatValue() == b.getFloatValue());
				break;
			case kStringParam:	
				r = (getStringValue() == b.getStringValue());
				break;
			case kSignalParam:	
				r = (getSignalValue() == b.getSignalValue());
				break;
		}
	}
	return r;
}

bool MLModelParam::operator!= (const MLModelParam& b) const
{
	return !operator==(b);
}

std::ostream& operator<< (std::ostream& out, const MLModelParam & r)
{
	switch(r.getType())
	{
		case MLModelParam::kUndefinedParam:
			out << "[undefined]";
			break;
		case MLModelParam::kFloatParam:	
			out << r.getFloatValue();
			break;
		case MLModelParam::kStringParam:	
			out << r.getStringValue();
			break;
		case MLModelParam::kSignalParam:	
			out << r.getSignalValue();
		break;
	}
	return out;
}


