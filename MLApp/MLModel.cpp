
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLModel.h"

// ----------------------------------------------------------------
// MLModelListener implementation

// called by the Model to notify us that one parameter has changed.
// if the param is new, or the value has changed, we mark the state as changed. 
//
void MLModelListener::modelParamChanged(MLSymbol paramSym)
{
	// if the param does not exist in the map yet, this lookup will add it. 
	ModelParamState& state = mModelParamStates[paramSym];

	// if not initialized, always mark as changed. 
	if(!state.mInitialized)
	{
		state.mChangedSinceUpdate = true;
		state.mInitialized = true;
		return;
	}
	else // check for value change. 
	{
		const MLModelParam& modelValue = mpModel->getModelParam(paramSym);
		if(modelValue != state.mValue)
		{
			state.mChangedSinceUpdate = true;
		}
	}
}

void MLModelListener::updateChangedParams() 
{	
	// for all model parameters we know about 
	std::map<MLSymbol, ModelParamState>::iterator it;
	for(it = mModelParamStates.begin(); it != mModelParamStates.end(); it++)
	{
		MLSymbol key = it->first;
		ModelParamState& state = it->second;
		
		if(state.mChangedSinceUpdate)
		{
			const MLModelParam& modelValue = mpModel->getModelParam(key);
			doParamChangeAction(key, state.mValue, modelValue);
			state.mChangedSinceUpdate = false;
			state.mValue = modelValue;
		}
	}
}

void MLModelListener::updateAllParams() 
{	
	mpModel->broadcastAllParams();
	std::map<MLSymbol, ModelParamState>::iterator it;
	for(it = mModelParamStates.begin(); it != mModelParamStates.end(); it++)
	{
		ModelParamState& state = it->second;
		state.mChangedSinceUpdate = true;
	}
	updateChangedParams();
}

// ----------------------------------------------------------------
// MLModel implementation

MLModel::MLModel()
{
}

MLModel::~MLModel()
{
}

void MLModel::setModelParam(MLSymbol p, float v) 
{
	mParams[p].setValue(v);
	broadcastParam(p);
}

void MLModel::setModelParam(MLSymbol p, const std::string& v) 
{
	mParams[p].setValue(v);
	broadcastParam(p);
}

void MLModel::setModelParam(MLSymbol p, const MLSignal& v) 
{
	mParams[p].setValue(v);
	broadcastParam(p);
}

void MLModel::addParamListener(MLModelListener* pL) 
{ 
	mpListeners.push_back(pL); 
}

void MLModel::removeParamListener(MLModelListener* pToRemove) 
{ 
	std::list<MLModelListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLModelListener* pL = *it;
		if(pL == pToRemove)
		{
			mpListeners.erase(it);
			return;
		}
	}
}

void MLModel::broadcastParam(MLSymbol p) 
{
	std::list<MLModelListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLModelListener* pL = *it;
		pL->modelParamChanged(p);
	}
}

void MLModel::broadcastAllParams()
{
	MLModelParameterMap::iterator it;
	for(it = mParams.begin(); it != mParams.end(); it++)
	{
		MLSymbol p = it->first;
		broadcastParam(p); 
	}
}

