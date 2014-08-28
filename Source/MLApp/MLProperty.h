
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PROPERTY__
#define __ML_PROPERTY__

#include <string>
#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLDebug.h"

// MLProperty: a modifiable property. Properties have three types: float, string, and signal.
// Properties start out with undefined type. Once a type is assigned, properties cannot change type.

class MLProperty
{
public:
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
	Colour getValueAsColor() const;
    
	void setValue(const MLProperty& v);
	void setValue(const float& v);
	void setValue(const std::string& v);
	void setValue(const MLSignal& v);
	void setValue(const Colour& v);
	
	bool operator== (const MLProperty& b) const;
	bool operator!= (const MLProperty& b) const;
	Type getType() const { return mType; }
	
	bool operator<< (const MLProperty& b) const;
	
private:
	Type mType;
	union
	{
		float mFloatVal;
		std::string* mpStringVal;
		MLSignal* mpSignalVal;
	}   mVal;
};

// utilities

std::ostream& operator<< (std::ostream& out, const MLProperty & r);

// MLPropertySet: a Set of Properties. Property names are stored as keys to the property map.

class MLPropertyListener;

class MLPropertySet
{	
public:
	MLPropertySet();
	virtual ~MLPropertySet();
    
	const MLProperty& getProperty(MLSymbol p) const;
	const float& getFloatProperty(MLSymbol p) const;
	const std::string& getStringProperty(MLSymbol p) const;
	const MLSignal& getSignalProperty(MLSymbol p) const;
	Colour getColorProperty(MLSymbol p) const;
    
	template <typename T>
	void setProperty(MLSymbol p, T v)
	{
		mProperties[p].setValue(v);
		broadcastProperty(p, false);
	}

	template <typename T>
	void setPropertyImmediate(MLSymbol p, T v)
	{
		mProperties[p].setValue(v);
		broadcastProperty(p, true);
	}

	void addPropertyListener(MLPropertyListener* pL);
	void removePropertyListener(MLPropertyListener* pToRemove);
    void broadcastAllProperties();
    
	static const MLProperty nullProperty;
	
private:
	std::map<MLSymbol, MLProperty> mProperties;
	std::list<MLPropertyListener*> mpListeners;
	void broadcastProperty(MLSymbol p, bool immediate);
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
		if(!mpPropertyOwner) return;
		mpPropertyOwner->removePropertyListener(this);
		mpPropertyOwner = nullptr;
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

#endif // __ML_PROPERTY__

