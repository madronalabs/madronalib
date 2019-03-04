
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <string>
#include <list>
#include <map>
#include "../matrix/MLSignal.h"
#include "MLSymbol.h"

#include "MLText.h"

// MLProperty: a modifiable property. Properties have four types: undefined, float, text, and signal.
// TODO rename to "Value"?
// Value / ValueSet / 

// TODO: instead of using MLSignal directly here as a type, make a blob type
// and utilities (in MLSignal) for conversion. This lets the current "model" code go into "app"
// because it doesn't depend on DSP math anymore, and increases reusability.

class MLProperty
{
public:
	static const MLSignal nullSignal;

	enum Type
	{
		kUndefinedProperty	= 0,
		kFloatProperty	= 1,
		kTextProperty = 2,
		kSignalProperty = 3
	};

	MLProperty();
	MLProperty(const MLProperty& other);
	MLProperty& operator= (const MLProperty & other);
	MLProperty(float v);
	MLProperty(int v);
	MLProperty(long v);
	MLProperty(double v);
	MLProperty(const ml::Text& t); 
	MLProperty(const char* t); 
	MLProperty(const MLSignal& s);

	// signal type constructor via initializer_list
	MLProperty (std::initializer_list<float> values)
	{
		*this = MLProperty(MLSignal(values));
	}

	~MLProperty();
    	
	inline const float getFloatValue() const
	{
		return mFloatVal;
		// static const float nullFloat = 0.f;
		// return (mType == kFloatProperty) ? mFloatVal : nullFloat;
	}
	
	inline const ml::Text getTextValue() const
	{
		return (mType == kTextProperty) ? (mTextVal) : ml::Text();
	}
	
	inline const MLSignal& getSignalValue() const
	{
		return (mType == kSignalProperty) ? (mSignalVal) : nullSignal;
	}
	
	// For each type of property, a setValue method must exist
	// to set the value of the property to that of the argument.
	//
	// For each type of property, if the size of the argument is equal to the
	// size of the current value, the value must be modified in place.
	// This guarantee keeps DSP graphs from allocating memory as they run.
	void setValue(const MLProperty& v);
	void setValue(const float& v);
	void setValue(const int& v);
	void setValue(const long& v);
	void setValue(const double& v);
	void setValue(const ml::Text& v);
	void setValue(const char* const v);
	void setValue(const MLSignal& v);
	
	bool operator== (const MLProperty& b) const;
	bool operator!= (const MLProperty& b) const;
	Type getType() const { return mType; }
	
	bool operator<< (const MLProperty& b) const;
	
private:
	// TODO reduce storage requirements-- this is a minimal-code start
	Type mType;
	float mFloatVal;
	ml::Text mTextVal;
	MLSignal mSignalVal;
};

// utilities

std::ostream& operator<< (std::ostream& out, const MLProperty & r);

struct MLPropertyChange
{
	ml::Symbol mName;
	MLProperty mValue;
};


