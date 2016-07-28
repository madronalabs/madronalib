
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPropertyListener.h"

#pragma mark MLPropertySet

const MLProperty MLPropertySet::nullProperty;

MLPropertySet::MLPropertySet() : mAllowNewProperties(true)
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

const MLProperty& MLPropertySet::getProperty(ml::Symbol p) const
{
	static const MLProperty nullProperty; // TODO remove this?
	
	std::map<ml::Symbol, MLProperty>::const_iterator it = mProperties.find(p);
	if(it != mProperties.end())
	{
		return it->second;
	}
	else
	{
		return nullProperty;
	}
}

const float MLPropertySet::getFloatProperty(ml::Symbol p) const
{
	static const float nullFloat = 0.f;

	std::map<ml::Symbol, MLProperty>::const_iterator it = mProperties.find(p);
	if(it != mProperties.end())
	{
		return it->second.getFloatValue();
	}
	else
	{
		return nullFloat;
	}
}

const ml::Text MLPropertySet::getTextProperty(ml::Symbol p) const
{
	std::map<ml::Symbol, MLProperty>::const_iterator it = mProperties.find(p);
	if(it != mProperties.end())
	{
		return it->second.getTextValue();
	}
	else
	{
		return ml::Text();
	}
}

const MLSignal& MLPropertySet::getSignalProperty(ml::Symbol p) const
{
	std::map<ml::Symbol, MLProperty>::const_iterator it = mProperties.find(p);
	if(it != mProperties.end())
	{
		return it->second.getSignalValue();
	}
	else
	{
		return MLProperty::nullSignal;
	}
}

// TODO check for duplicates! That could lead to a crash.
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

void MLPropertySet::broadcastProperty(ml::Symbol p, bool immediate)
{
	std::list<MLPropertyListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLPropertyListener* pL = *it;
		pL->propertyChanged(p, immediate);
	}
}

void MLPropertySet::broadcastPropertyExcludingListener(ml::Symbol p, bool immediate, MLPropertyListener* pListenerToExclude)
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
	std::map<ml::Symbol, MLProperty>::const_iterator it;
	for(it = mProperties.begin(); it != mProperties.end(); it++)
	{
		ml::Symbol p = it->first;
		
		// TODO cut down on some of this broadcasting!
		// debug() << "BROADCASTING: " << p << "\n";

		broadcastProperty(p, false);
	}
}

#pragma mark MLPropertyListener

void MLPropertyListener::updateChangedProperties()
{
    if(!mpPropertyOwner) return;
	// for all model parameters we know about
	std::map<ml::Symbol, PropertyState>::iterator it;
	for(it = mPropertyStates.begin(); it != mPropertyStates.end(); it++)
	{
		ml::Symbol key = it->first;
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

	// mark all states as changed
	std::map<ml::Symbol, PropertyState>::iterator it;
	for(it = mPropertyStates.begin(); it != mPropertyStates.end(); it++)
	{
		PropertyState& state = it->second;
		state.mChangedSinceUpdate = true;
	}
	
	updateChangedProperties();
}

void MLPropertyListener::propertyChanged(ml::Symbol propName, bool immediate)
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

