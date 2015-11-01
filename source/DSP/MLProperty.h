
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PROPERTY__
#define __ML_PROPERTY__

#include <string>
#include <list>
#include <map>
#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLDebug.h"

// MLProperty: a modifiable property. Properties have four types: undefined, float, string, and signal.

class MLProperty
{
public:
	static const std::string nullString;
	static const MLSignal nullSignal;

	enum Type
	{
		kUndefinedProperty	= 0,
		kFloatProperty	= 1,
		kStringProperty = 2,
		kSignalProperty = 3
	};

	MLProperty();
	MLProperty(const MLProperty& other);
	MLProperty& operator= (const MLProperty & other);
	MLProperty(float v);
	MLProperty(const std::string& s);
	MLProperty(const MLSignal& s);
	~MLProperty();
    
	const float& getFloatValue() const;
	const std::string& getStringValue() const;
	const MLSignal& getSignalValue() const;
    
	// For each type of property, a setValue method must exist
	// to set the value of the property to that of the argument.
	//
	// For each type of property, if the size of the argument is equal to the
	// size of the current value, the value must be modified in place.
	// This guarantee keeps DSP graphs from allocating memory as they run.
	void setValue(const MLProperty& v);
	void setValue(const float& v);
	void setValue(const std::string& v);
	void setValue(const MLSignal& v);
	
	bool operator== (const MLProperty& b) const;
	bool operator!= (const MLProperty& b) const;
	Type getType() const { return mType; }
	
	bool operator<< (const MLProperty& b) const;
	
private:
	// TODO reduce storage requirements-- this is a minimal-code start
	// TODO stop using std::string, which seems to be causing threading issues
	Type mType;
	float mFloatVal;
	std::string mStringVal;
	MLSignal mSignalVal;
};

// utilities

std::ostream& operator<< (std::ostream& out, const MLProperty & r);

// MLPropertySet: a Set of Properties. Property names are stored as keys to the property map.

class MLPropertyListener;

class MLPropertySet
{	
    friend class MLPropertyListener;
	
public:
	MLPropertySet();
	virtual ~MLPropertySet();
    
	const MLProperty& getProperty(MLSymbol p) const;
	const float& getFloatProperty(MLSymbol p) const;
	const std::string& getStringProperty(MLSymbol p) const;
	const MLSignal& getSignalProperty(MLSymbol p) const;
    
	// set the property and allow it to propagate to Listeners the next time
	// each Listener calls updateChangedProperties().
	template <typename T>
	void setProperty(MLSymbol p, T v)
	{
		mProperties[p].setValue(v);
		broadcastProperty(p, false);
	}

	// set the property and propagate to Listeners immediately.
	template <typename T>
	void setPropertyImmediate(MLSymbol p, T v)
	{
		mProperties[p].setValue(v);
		broadcastProperty(p, true);
	}
	
	// set the property and propagate to Listeners immediately,
	// except for the argument Listener pL
	template <typename T>
	void setPropertyImmediateExcludingListener(MLSymbol p, T v, MLPropertyListener* pL)
	{
		mProperties[p].setValue(v);
		broadcastPropertyExcludingListener(p, true, pL);
	}
	
    void broadcastAllProperties();
    
	static const MLProperty nullProperty;

protected:
	void addPropertyListener(MLPropertyListener* pL);
	void removePropertyListener(MLPropertyListener* pToRemove);
	
private:
	std::map<MLSymbol, MLProperty> mProperties;
	std::list<MLPropertyListener*> mpListeners;
	
	void broadcastProperty(MLSymbol p, bool immediate);
	void broadcastPropertyExcludingListener(MLSymbol p, bool immediate, MLPropertyListener* pListenerToExclude);
};

// MLPropertyListeners are notified when a Property of an MLPropertySet changes. They do something in
// response by overriding doPropertyChangeAction().

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
	virtual void doPropertyChangeAction(MLSymbol param, const MLProperty & newVal) = 0;
	
	// call periodically to do actions for any properties that have changed since the last call.
	void updateChangedProperties();
	
	// force an update of all properties.
	void updateAllProperties();
    
protected:

    // called by a PropertySet to notify us that one property has changed.
	// if the property is new, or the value has changed, we mark the state as changed.
	// If immediate is true and the state has changed, doPropertyChangeAction() will be called.
	void propertyChanged(MLSymbol p, bool immediate);
    
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
    
	std::map<MLSymbol, PropertyState> mPropertyStates;
	MLPropertySet* mpPropertyOwner;
};

typedef std::shared_ptr<MLPropertyListener> MLPropertyListenerPtr;

#endif // __ML_PROPERTY__

