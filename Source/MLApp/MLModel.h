
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MODEL__
#define __ML_MODEL__

#include <string>
#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLDebug.h"

// MLProperty: a modifiable property of a model. Model properties have three types: float, string, and signal.

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
    
    // set an existing attibute to a new value. values cannot change type.
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
	} mVal;
};

std::ostream& operator<< (std::ostream& out, const MLProperty & r);

typedef std::map<MLSymbol, MLProperty> MLPropertyMap;

class MLModel;

// ----------------------------------------------------------------
// MLModelListeners are notified when a Property changes.

class MLModelListener
{
public:
	MLModelListener(MLModel* m) : mpModel(m) {}
	virtual ~MLModelListener() {}
	
	// mark one property as changed.
	void propertyChanged(MLSymbol p);

	// do an action for any params with changed values.
	void updateChangedProperties();
	
	// force an update of all params.
	void updateAllProperties();

	virtual void doPropertyChangeAction(MLSymbol param, const MLProperty & oldVal, const MLProperty & newVal) = 0;
	
protected:	
	// represent the state of a model parameter relative to updates. 
	class ModelPropertyState
	{
	public:
		ModelPropertyState() : 
			mInitialized(false),
			mChangedSinceUpdate(true)
			{};
			
		~ModelPropertyState() {}
		
		bool mInitialized;  
		bool mChangedSinceUpdate; 
		MLProperty mValue;
	};
		
	std::map<MLSymbol, ModelPropertyState> mModelPropertyStates;

	MLModel* mpModel;
};

// ----------------------------------------------------------------

class MLModel
{
public:
	MLModel();
	virtual ~MLModel();	

	inline const MLProperty& getProperty(MLSymbol p) { return mProperties[p]; }

	inline float getFloatProperty(MLSymbol p) { return mProperties[p].getFloatValue(); }
	inline const std::string* getStringProperty(MLSymbol p) { return mProperties[p].getStringValue(); }
	inline const MLSignal* getSignalProperty(MLSymbol p) { return mProperties[p].getSignalValue(); }

	virtual void setProperty(MLSymbol p, float v);
	virtual void setProperty(MLSymbol p, const std::string& v);
	virtual void setProperty(MLSymbol p, const MLSignal& v);
	
	void addPropertyListener(MLModelListener* pL);
	void removePropertyListener(MLModelListener* pToRemove);
	void broadcastAllProperties();
	
protected:
	MLPropertyMap mProperties;

	std::list<MLModelListener*> mpListeners;
	void broadcastProperty(MLSymbol p);
	void broadcastStringParam(MLSymbol p);
};

#endif // __ML_MODEL__

