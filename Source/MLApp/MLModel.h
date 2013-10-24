
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MODEL__
#define __ML_MODEL__

#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLModelParam.h"

class MLModel;

// ----------------------------------------------------------------
// MLModelListeners are notified when any of a Model's parameters change. 

class MLModelListener
{
public:
	MLModelListener(MLModel* m) : mpModel(m) {}
	virtual ~MLModelListener() {}
	
	// mark one parameter as changed. 
	void modelParamChanged(MLSymbol p);

	// mark one parameter as changed. 
	void modelStringParamChanged(MLSymbol p);

	// do an action for any params with changed values.
	void updateChangedParams();
	
	// force an update of all params.
	void updateAllParams();

	virtual void doParamChangeAction(MLSymbol param, const MLModelParam & oldVal, const MLModelParam & newVal) = 0;
	
protected:	
	// represent the state of a model parameter relative to updates. 
	class ModelParamState
	{
	public:
		ModelParamState() : 
			mInitialized(false),
			mChangedSinceUpdate(true)
			{};
			
		~ModelParamState() {}
		
		bool mInitialized;  
		bool mChangedSinceUpdate; 
		MLModelParam mValue;
	};
		
	std::map<MLSymbol, ModelParamState> mModelParamStates;

	MLModel* mpModel;
};

// ----------------------------------------------------------------
#pragma mark MVC application attribute code

// TODO WHY did I call attributes "parameters"? Lots of renaming to do.

typedef std::map<MLSymbol, MLModelParam> MLModelParameterMap;

class MLModel
{
public:
	MLModel();
	virtual ~MLModel();	

	inline const MLModelParam& getModelParam(MLSymbol p) { return mParams[p]; }

	inline float getModelFloatParam(MLSymbol p) { return mParams[p].getFloatValue(); }
	inline const std::string* getModelStringParam(MLSymbol p) { return mParams[p].getStringValue(); }
	inline const MLSignal* getModelSignalParam(MLSymbol p) { return mParams[p].getSignalValue(); }

	virtual void setModelParam(MLSymbol p, float v);
	virtual void setModelParam(MLSymbol p, const std::string& v) ;
	virtual void setModelParam(MLSymbol p, const MLSignal& v) ;
	
	void addParamListener(MLModelListener* pL);
	void removeParamListener(MLModelListener* pToRemove);
	void broadcastAllParams();
	
protected:
	MLModelParameterMap mParams;

	std::list<MLModelListener*> mpListeners;
	void broadcastParam(MLSymbol p);
	void broadcastStringParam(MLSymbol p);
};

#endif // __ML_MODEL__

