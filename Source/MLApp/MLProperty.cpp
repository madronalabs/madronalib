
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProperty.h"

#pragma mark MLProperty

const std::string MLProperty::nullString;
const MLSignal MLProperty::nullSignal;

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
			mVal.mpStringVal = new std::string(other.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			mVal.mpSignalVal = new MLSignal(other.getSignalValue());
			break;
		default:
			mVal.mpStringVal = 0;
			break;
	}
}

MLProperty& MLProperty::operator= (const MLProperty& other)
{
	if(mType != kUndefinedProperty)
	{
		deallocate();
	}
	mType = other.getType();
	switch(mType)
	{
		case MLProperty::kFloatProperty:
			mVal.mFloatVal = other.getFloatValue();
			break;
		case MLProperty::kStringProperty:
			mVal.mpStringVal = new std::string(other.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			mVal.mpSignalVal = new MLSignal(other.getSignalValue());
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
	deallocate();
}

void MLProperty::deallocate()
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
			mType = kUndefinedProperty;
			mVal.mpStringVal = 0;
			break;
	}
}

const float& MLProperty::getFloatValue() const
{
	static const float nullFloat = 0.f;
	return (mType == kFloatProperty) ? mVal.mFloatVal : nullFloat;
}

const std::string& MLProperty::getStringValue() const
{
	return (mType == kStringProperty) ? (*mVal.mpStringVal) : nullString;
}

const MLSignal& MLProperty::getSignalValue() const
{
	return (mType == kSignalProperty) ? (*mVal.mpSignalVal) : nullSignal;
}

void MLProperty::setValue(const float& v)
{
	if(mType == kFloatProperty)
	{
		mVal.mFloatVal = v;
	}
	else
	{
		deallocate();
		mType = kFloatProperty;
		mVal.mFloatVal = v;
	}
}

void MLProperty::setValue(const std::string& v)
{
	if(mType == kStringProperty)
	{
		std::string& s = *mVal.mpStringVal;
		s.replace(s.begin(), s.end(), v);
	}
	else
	{
		deallocate();
		mType = kStringProperty;
		mVal.mpStringVal = new std::string(v);
	}
}

void MLProperty::setValue(const MLSignal& v)
{
	if(mType == kSignalProperty)
	{
		*mVal.mpSignalVal = v;
	}
	else
	{
		deallocate();
		mType = kSignalProperty;
		mVal.mpSignalVal = new MLSignal(v);
	}
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
		case MLProperty::kStringProperty:
			out << (r.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			out << (r.getSignalValue());
            break;
	}
	return out;
}

#pragma mark MLPropertySet

const MLProperty MLPropertySet::nullProperty;

MLPropertySet::MLPropertySet()
{
}

MLPropertySet::~MLPropertySet()
{
    std::list<MLPropertyListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLPropertyListener* pL = *it;
		pL->propertyOwnerClosing();
    }
    mpListeners.clear();
}

const MLProperty& MLPropertySet::getProperty(MLSymbol p) const
{
	static const MLProperty nullProperty;
	std::map<MLSymbol, MLProperty>::const_iterator it = mProperties.find(p);
	if(it != mProperties.end())
	{
		return it->second;
	}
	else
	{
		return nullProperty;
	}
}

const float& MLPropertySet::getFloatProperty(MLSymbol p) const
{
	static const float nullFloat = 0.f;
	std::map<MLSymbol, MLProperty>::const_iterator it = mProperties.find(p);
	if(it != mProperties.end())
	{
		return it->second.getFloatValue();
	}
	else
	{
		return nullFloat;
	}
}

const std::string& MLPropertySet::getStringProperty(MLSymbol p) const
{
	std::map<MLSymbol, MLProperty>::const_iterator it = mProperties.find(p);
	if(it != mProperties.end())
	{
		return it->second.getStringValue();
	}
	else
	{
		return MLProperty::nullString;
	}
}

const MLSignal& MLPropertySet::getSignalProperty(MLSymbol p) const
{
	std::map<MLSymbol, MLProperty>::const_iterator it = mProperties.find(p);
	if(it != mProperties.end())
	{
		return it->second.getSignalValue();
	}
	else
	{
		return MLProperty::nullSignal;
	}
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

void MLPropertySet::broadcastProperty(MLSymbol p, bool immediate)
{
	std::list<MLPropertyListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLPropertyListener* pL = *it;
		pL->propertyChanged(p, immediate);
	}
}

void MLPropertySet::broadcastPropertyExcludingListener(MLSymbol p, bool immediate, MLPropertyListener* pListenerToExclude)
{
	std::list<MLPropertyListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLPropertyListener* pL = *it;
		if(pL != pListenerToExclude)
		{
			pL->propertyChanged(p, immediate);
		}
	}
}

void MLPropertySet::broadcastAllProperties()
{
	std::map<MLSymbol, MLProperty>::const_iterator it;
	for(it = mProperties.begin(); it != mProperties.end(); it++)
	{
		MLSymbol p = it->first;
		broadcastProperty(p, true);
	}
}

#pragma mark MLPropertyListener

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

void MLPropertyListener::propertyChanged(MLSymbol propName, bool immediate)
{
    if(!mpPropertyOwner) return;
    
	// if the property does not exist in the map yet, this lookup will add it.
	PropertyState& state = mPropertyStates[propName];
	
	// check for change in property. Note that this also compares signals and strings, which may possibly be slow.
    const MLProperty& ownerValue = mpPropertyOwner->getProperty(propName);
	
	if(ownerValue != state.mValue)
    {
		if(immediate)
		{
			doPropertyChangeAction(propName, ownerValue);
			state.mValue = ownerValue;
		}
		else
		{
			state.mChangedSinceUpdate = true;
		}
    }
}

void MLPropertyListener::propertyOwnerClosing()
{
    if(!mpPropertyOwner) return;
    mpPropertyOwner = nullptr;
}
