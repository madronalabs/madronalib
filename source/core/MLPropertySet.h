
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <string>
#include <list>
#include <map>
#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLText.h"
//#include "MLDebug.h"
#include "MLProperty.h"

// MLPropertySet: a Set of Properties. Property names are stored as keys to the property map.

// TODO with the Listener stuff this class wraps up two very different concepts: storage and
// communication. That seems like a bad design. communication should be a different class from the
// propertySet - a kind of decorator that does the sending maybe.
//
// new object: a kind of messageSender that sends changes in properties. 
// an adaptor from changes in data to transmissions. 
// d(data)/d(time) = changes in data. Reversible. must be lossless.
// think of change recorder as a kind of filter. Then transmitter is another generic thing that can be composed with it.

class MLPropertyListener;

class MLPropertySet
{	
	friend class MLPropertyListener;
	
public:
	MLPropertySet();
	virtual ~MLPropertySet();

	const MLProperty& getProperty(ml::Symbol p) const;
	const float getFloatProperty(ml::Symbol p) const;
	const ml::Text getTextProperty(ml::Symbol p) const;
	const MLSignal& getSignalProperty(ml::Symbol p) const;
	
	// set the property and allow it to propagate to Listeners the next time
	// each Listener calls updateChangedProperties().
	template <typename T>
	void setProperty(ml::Symbol p, const T& v)
	{
		if(mAllowNewProperties)
		{
			mProperties[p].setValue(v);
			broadcastProperty(p, false);
		}
		else
		{
			std::map<ml::Symbol, MLProperty>::const_iterator it = mProperties.find(p);
			if(it != mProperties.end())
			{
				mProperties[p].setValue(v);
				broadcastProperty(p, false);
			}
		}
	}
	
	// set the property and propagate to Listeners immediately.
	template <typename T>
	void setPropertyImmediate(ml::Symbol p, const T& v)
	{
		if(mAllowNewProperties)
		{
			mProperties[p].setValue(v);
			broadcastProperty(p, true);
		}
		else
		{
			std::map<ml::Symbol, MLProperty>::const_iterator it = mProperties.find(p);
			if(it != mProperties.end())
			{
				mProperties[p].setValue(v);
				broadcastProperty(p, true);
			}
		}
	}
	
	// set the property and propagate to Listeners immediately,
	// except for the argument Listener pL
	template <typename T>
	void setPropertyImmediateExcludingListener(ml::Symbol p, const T& v, MLPropertyListener* pL)
	{
		if(mAllowNewProperties)
		{
			mProperties[p].setValue(v);
			broadcastPropertyExcludingListener(p, true, pL);
		}
		else
		{
			std::map<ml::Symbol, MLProperty>::const_iterator it = mProperties.find(p);
			if(it != mProperties.end())
			{
				mProperties[p].setValue(v);
				broadcastPropertyExcludingListener(p, true, pL);
			}
		}
	}
	
	void broadcastAllProperties();
	void allowNewProperties(bool b) { mAllowNewProperties = b; }
	
	static const MLProperty nullProperty;
	
	void dumpProperties()
	{
		std::map<ml::Symbol, MLProperty>::const_iterator it;
		std::cout<< "\n" << mProperties.size() << " properties: \n";
		for(it = mProperties.begin(); it != mProperties.end(); it++)
		{
			ml::Symbol name = it->first;
			MLProperty val = it->second;
			std::cout << name << ": " << val << "\n";
		}		
	}
	
protected:
	void addPropertyListener(MLPropertyListener* pL);
	void removePropertyListener(MLPropertyListener* pToRemove);
	
private:
	std::map<ml::Symbol, MLProperty> mProperties;
	std::list<MLPropertyListener*> mpListeners;
	bool mAllowNewProperties;
	
	void broadcastProperty(ml::Symbol p, bool immediate);
	void broadcastPropertyExcludingListener(ml::Symbol p, bool immediate, MLPropertyListener* pListenerToExclude);
};

// MLPropertyListeners are notified when a Property of an MLPropertySet changes. They do something in
// response by overriding doPropertyChangeAction().

// TODO make generic message passing implementation!



class MLPropertyListener
{
	friend class MLPropertySet;
public:
	MLPropertyListener(MLPropertySet* m) : mpPropertyOwner(m)
	{
		mpPropertyOwner->addPropertyListener(this);
	}
	
	virtual ~MLPropertyListener()
	{
		if(mpPropertyOwner)
		{
			mpPropertyOwner->removePropertyListener(this);
		}
	}
	
	// override to do whatever this PropertyListener needs to do based on the values of properties.
	virtual void doPropertyChangeAction(ml::Symbol param, const MLProperty & newVal) = 0;
	
	// call periodically to do actions for any properties that have changed since the last call.
	void updateChangedProperties();
	
	// force an update of all properties.
	void updateAllProperties();
	
protected:
	
	// called by a PropertySet to notify us that one property has changed.
	// if the property is new, or the value has changed, we mark the state as changed.
	// If immediate is true and the state has changed, doPropertyChangeAction() will be called.
	void propertyChanged(ml::Symbol p, bool immediate);
	
	// Must be called by the Property owner to notify us in the event it is going away.
	void propertyOwnerClosing();
	
	// PropertyStates represent the state of a single property relative to updates.
	class PropertyState
	{
	public:
		PropertyState() : mChangedSinceUpdate(true) {}
		~PropertyState() {}
		
		bool mChangedSinceUpdate;
		MLProperty mValue;
	};
	
	std::map<ml::Symbol, PropertyState> mPropertyStates;
	MLPropertySet* mpPropertyOwner;
};

typedef std::shared_ptr<MLPropertyListener> MLPropertyListenerPtr;
