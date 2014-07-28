
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProperty.h"

// --------------------------------------------------------------------------------
// MLProperty

MLProperty::MLProperty() :
mType(kUndefinedProperty)
{
	mVal.mpStringVal = 0;
}

MLProperty::MLProperty(const MLProperty& other) :
mType(other.getType())
{
	switch(mType)
	{
		case MLProperty::kFloatProperty:
			mVal.mFloatVal = other.getFloatValue();
			break;
		case MLProperty::kStringProperty:
			mVal.mpStringVal = new std::string(*other.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			mVal.mpSignalVal = new MLSignal(*other.getSignalValue());
			break;
		default:
			mVal.mpStringVal = 0;
			break;
	}
}

MLProperty& MLProperty::operator= (const MLProperty& other)
{
	mType = other.getType();
	switch(mType)
	{
		case MLProperty::kFloatProperty:
			mVal.mFloatVal = other.getFloatValue();
			break;
		case MLProperty::kStringProperty:
			mVal.mpStringVal = new std::string(*other.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			mVal.mpSignalVal = new MLSignal(*other.getSignalValue());
			break;
		default:
			mVal.mpStringVal = 0;
			break;
	}
	return *this;
}

MLProperty::MLProperty(float v) :
mType(kFloatProperty)
{
	mVal.mFloatVal = v;
}

MLProperty::MLProperty(const std::string& s) :
mType(kStringProperty)
{
	mVal.mpStringVal = new std::string(s);
}

MLProperty::MLProperty(const MLSignal& s) :
mType(kSignalProperty)
{
	mVal.mpSignalVal = new MLSignal(s);
}

MLProperty::~MLProperty()
{
	switch(mType)
	{
		case kStringProperty:
			delete mVal.mpStringVal;
			break;
		case kSignalProperty:
			delete mVal.mpSignalVal;
			break;
		default:
			break;
	}
}

float MLProperty::getFloatValue() const
{
	return mVal.mFloatVal;
}

const std::string* MLProperty::getStringValue() const
{
	return (mVal.mpStringVal);
}

const MLSignal* MLProperty::getSignalValue() const
{
	return (mVal.mpSignalVal);
}

void MLProperty::setValue(float v)
{
	if(mType == kUndefinedProperty)
		mType = kFloatProperty;
	if(mType == kFloatProperty)
	{
		mVal.mFloatVal = v;
	}
	else
	{
		debug() << "MLProperty::setValue: type mismatch! Expected float.\n";
	}
}

void MLProperty::setValue(const std::string& v)
{
	if(mType == kUndefinedProperty)
		mType = kStringProperty;
	if(mType == kStringProperty)
	{
		delete mVal.mpStringVal;
		mVal.mpStringVal = new std::string(v);
	}
	else
	{
		debug() << "MLProperty::setValue: type mismatch! Expected string.\n";
	}
}

void MLProperty::setValue(const MLSignal& v)
{
	if(mType == kUndefinedProperty)
		mType = kSignalProperty;
	if(mType == kSignalProperty)
	{
		delete mVal.mpSignalVal;
		mVal.mpSignalVal = new MLSignal(v);
	}
	else
	{
		debug() << "MLProperty::setValue: type mismatch! Expected signal.\n";
	}
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
			case kStringProperty:
				r = (getStringValue() == b.getStringValue());
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
		case MLProperty::kStringProperty:
			out << *(r.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			out << *(r.getSignalValue());
            break;
	}
	return out;
}

// --------------------------------------------------------------------------------
// MLPropertyListener

// called by a PropertySet to notify us that one property has changed.
// if the property is new, or the value has changed, we mark the state as changed.
//
void MLPropertyListener::propertyChanged(MLSymbol paramSym)
{
    if(!mpPropertyOwner) return;
    
	// if the property does not exist in the map yet, this lookup will add it.
	PropertyState& state = mPropertyStates[paramSym];
    
    const MLProperty& modelValue = mpPropertyOwner->getProperty(paramSym);
    if(modelValue != state.mValue)
    {
        state.mChangedSinceUpdate = true;
    }
}

void MLPropertyListener::updateChangedProperties()
{
    if(!mpPropertyOwner) return;
	// for all model parameters we know about
	std::map<MLSymbol, PropertyState>::iterator it;
	for(it = mPropertyStates.begin(); it != mPropertyStates.end(); it++)
	{
		MLSymbol key = it->first;
		PropertyState& state = it->second;
		
		if(state.mChangedSinceUpdate)
		{
			const MLProperty& newValue = mpPropertyOwner->getProperty(key);
			doPropertyChangeAction(key, newValue);
			state.mChangedSinceUpdate = false;
			state.mValue = newValue;
		}
	}
}

void MLPropertyListener::updateAllProperties()
{
    if(!mpPropertyOwner) return;
	mpPropertyOwner->broadcastAllProperties();
	std::map<MLSymbol, PropertyState>::iterator it;
	for(it = mPropertyStates.begin(); it != mPropertyStates.end(); it++)
	{
		PropertyState& state = it->second;
		state.mChangedSinceUpdate = true;
	}
	updateChangedProperties();
}

void MLPropertyListener::ownerClosing()
{
    mpPropertyOwner = nullptr;
}

// --------------------------------------------------------------------------------
// MLPropertySet

MLPropertySet::MLPropertySet()
{
}

MLPropertySet::~MLPropertySet()
{
    std::list<MLPropertyListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLPropertyListener* pL = *it;
		pL->ownerClosing();
    }
}

void MLPropertySet::setProperty(MLSymbol p, float v)
{
	mProperties[p].setValue(v);
	broadcastProperty(p);
}

void MLPropertySet::setProperty(MLSymbol p, const std::string& v)
{
	mProperties[p].setValue(v);
	broadcastProperty(p);
}

void MLPropertySet::setProperty(MLSymbol p, const MLSignal& v)
{
	mProperties[p].setValue(v);
	broadcastProperty(p);
}

void MLPropertySet::addPropertyListener(MLPropertyListener* pL)
{
	mpListeners.push_back(pL);
}

void MLPropertySet::removePropertyListener(MLPropertyListener* pToRemove)
{
	std::list<MLPropertyListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLPropertyListener* pL = *it;
		if(pL == pToRemove)
		{
			mpListeners.erase(it);
			return;
		}
	}
}

void MLPropertySet::broadcastProperty(MLSymbol p)
{
	std::list<MLPropertyListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLPropertyListener* pL = *it;
		pL->propertyChanged(p);
	}
}

void MLPropertySet::broadcastAllProperties()
{
	std::map<MLSymbol, MLProperty>::const_iterator it;
	for(it = mProperties.begin(); it != mProperties.end(); it++)
	{
		MLSymbol p = it->first;
		broadcastProperty(p);
	}
}

// --------------------------------------------------------------------------------
// MLPropertyModifier

void MLPropertyModifier::requestPropertyChange(MLSymbol p, float v)
{
    if(mpPropertyOwner)
    {
        mpPropertyOwner->setProperty(p, v);
    }
}

void MLPropertyModifier::requestPropertyChange(MLSymbol p, const std::string& v)
{
    if(mpPropertyOwner)
    {
        mpPropertyOwner->setProperty(p, v);
    }
}

void MLPropertyModifier::requestPropertyChange(MLSymbol p, const MLSignal& v)
{
    if(mpPropertyOwner)
    {
        mpPropertyOwner->setProperty(p, v);
    }
}

