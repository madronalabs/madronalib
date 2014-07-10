
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MODEL__
#define __ML_MODEL__

#include <string>
#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLDebug.h"

// MLModelProperty: a modifiable property of a model. Model properties have three types: float, string, and signal.

class MLModelProperty
{
public:
	enum eType
	{
		kUndefinedProperty	= 0,
		kFloatProperty	= 1,
		kStringProperty = 2,
		kSignalProperty = 3
	};
    
	MLModelProperty();
	MLModelProperty(const MLModelProperty& other);
	MLModelProperty& operator= (const MLModelProperty & other);
	MLModelProperty(float v);
	MLModelProperty(const std::string& s);
	MLModelProperty(const MLSignal& s);
	~MLModelProperty();
    
	float getFloatValue() const;
	const std::string* getStringValue() const;
	const MLSignal* getSignalValue() const;
    
    // set an existing attibute to a new value. values cannot change type.
	void setValue(float v);
	void setValue(const std::string& v);
	void setValue(const MLSignal& v);
	
	bool operator== (const MLModelProperty& b) const;
	bool operator!= (const MLModelProperty& b) const;
	eType getType() const { return mType; }
	
	bool operator<< (const MLModelProperty& b) const;
	
private:
	
	eType mType;
	union
	{
		float mFloatVal;
		std::string* mpStringVal;
		MLSignal* mpSignalVal;
	} mVal;
};

std::ostream& operator<< (std::ostream& out, const MLModelProperty & r);

class MLModel;

// ----------------------------------------------------------------
// MLModelListeners are notified when any of a Model's properties change.

class MLModelListener
{
public:
	MLModelListener(MLModel* m) : mpModel(m) {}
	virtual ~MLModelListener() {}
	
	// mark one parameter as changed. 
	void modelPropertyChanged(MLSymbol p);

	// mark one parameter as changed. 
	void modelStringParamChanged(MLSymbol p);

	// do an action for any params with changed values.
	void updateChangedProperties();
	
	// force an update of all params.
	void updateAllProperties();

	virtual void doPropertyChangeAction(MLSymbol param, const MLModelProperty & oldVal, const MLModelProperty & newVal) = 0;
	
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
		MLModelProperty mValue;
	};
		
	std::map<MLSymbol, ModelPropertyState> mModelPropertyStates;

	MLModel* mpModel;
};

// ----------------------------------------------------------------

typedef std::map<MLSymbol, MLModelProperty> MLModelPropertyMap;

class MLModel
{
public:
	MLModel();
	virtual ~MLModel();	

	inline const MLModelProperty& getModelProperty(MLSymbol p) { return mProperties[p]; }

	inline float getModelFloatParam(MLSymbol p) { return mProperties[p].getFloatValue(); }
	inline const std::string* getModelStringParam(MLSymbol p) { return mProperties[p].getStringValue(); }
	inline const MLSignal* getModelSignalParam(MLSymbol p) { return mProperties[p].getSignalValue(); }

	virtual void setModelProperty(MLSymbol p, float v);
	virtual void setModelProperty(MLSymbol p, const std::string& v);
	virtual void setModelProperty(MLSymbol p, const MLSignal& v);
	
	void addPropertyListener(MLModelListener* pL);
	void removePropertyListener(MLModelListener* pToRemove);
	void broadcastAllProperties();
	
protected:
	MLModelPropertyMap mProperties;

	std::list<MLModelListener*> mpListeners;
	void broadcastProperty(MLSymbol p);
	void broadcastStringParam(MLSymbol p);
};

#endif // __ML_MODEL__

