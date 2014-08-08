
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
	enum eType
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
    
	float getFloatValue() const;
	const std::string* getStringValue() const;
	const MLSignal* getSignalValue() const;
    
	void setValue(float v);
	void setValue(const std::string& v);
	void setValue(const MLSignal& v);
	
	bool operator== (const MLProperty& b) const;
	bool operator!= (const MLProperty& b) const;
	eType getType() const { return mType; }
	
	bool operator<< (const MLProperty& b) const;
	
private:
	eType mType;
	union
	{
		float mFloatVal;
		std::string* mpStringVal;
		MLSignal* mpSignalVal;
	}   mVal;
};

std::ostream& operator<< (std::ostream& out, const MLProperty & r);

class MLPropertyListener;
class MLPropertyModifier;

// a Set of Properties. Property names are stored here.

class MLPropertySet
{
    friend class MLPropertyModifier;
public:
	MLPropertySet();
	virtual ~MLPropertySet();
    
	inline const MLProperty& getProperty(MLSymbol p) { return mProperties[p]; }
	inline float getFloatProperty(MLSymbol p) { return mProperties[p].getFloatValue(); }
	inline const std::string* getStringProperty(MLSymbol p) { return mProperties[p].getStringValue(); }
	inline const MLSignal* getSignalProperty(MLSymbol p) { return mProperties[p].getSignalValue(); }
    
	void addPropertyListener(MLPropertyListener* pL);
	void removePropertyListener(MLPropertyListener* pToRemove);
    void broadcastAllProperties();
    
protected:
    void setProperty(MLSymbol p, float v);
    void setProperty(MLSymbol p, const std::string& v);
    void setProperty(MLSymbol p, const MLSignal& v);
	
private:
	std::map<MLSymbol, MLProperty> mProperties;
	std::list<MLPropertyListener*> mpListeners;
	void broadcastProperty(MLSymbol p);
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
        stopListening();
    }
    
	virtual void doPropertyChangeAction(MLSymbol param, const MLProperty & newVal) = 0;
	
	// do an action for any params with changed values.
	void updateChangedProperties();
	
	// force an update of all params.
	void updateAllProperties();
    
protected:
    // mark one property as changed.
	void propertyChanged(MLSymbol p);
    
    void stopListening();
    void propertyOwnerClosing();
    
	// represent the state of a property relative to updates.
	class PropertyState
	{
	public:
		PropertyState() :
        mChangedSinceUpdate(true)
        {};
        
		~PropertyState() {}
		
		bool mChangedSinceUpdate;
		MLProperty mValue;
	};
    
	std::map<MLSymbol, PropertyState> mPropertyStates;
	MLPropertySet* mpPropertyOwner;
};

// an MLPropertyModifier can request that PropertySets make changes to Properties.
// use, for example, to control a Model from a UI or recall it to a saved state.

class MLPropertyModifier
{
public:
	MLPropertyModifier(MLPropertySet* m) : mpPropertyOwner(m) {}
	virtual ~MLPropertyModifier() {}
    void requestPropertyChange(MLSymbol p, float v);
    void requestPropertyChange(MLSymbol p, const std::string& v);
    void requestPropertyChange(MLSymbol p, const MLSignal& v);
    
private:
    MLPropertySet* mpPropertyOwner;
};

#endif // __ML_PROPERTY__

