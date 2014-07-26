
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLModel.h"

static const std::string kNullModelPropertyString;
static const MLSignal kNullModelPropertySignal;

// ----------------------------------------------------------------
// MLProperty implementation


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
			mVal.mpStringVal = new std::string(*other.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			mVal.mpSignalVal = new MLSignal(*other.getSignalValue());
			break;
		default:
			mVal.mpStringVal = 0;
			break;
	}
}

MLProperty& MLProperty::operator= (const MLProperty& other)
{
	mType = other.getType();
	switch(mType)
	{
		case MLProperty::kFloatProperty:
			mVal.mFloatVal = other.getFloatValue();
			break;
		case MLProperty::kStringProperty:
			mVal.mpStringVal = new std::string(*other.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			mVal.mpSignalVal = new MLSignal(*other.getSignalValue());
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
	switch(mType)
	{
		case kStringProperty:
			delete mVal.mpStringVal;
			break;
		case kSignalProperty:
			delete mVal.mpSignalVal;
			break;
		default:
			break;
	}
}

float MLProperty::getFloatValue() const
{
	return mVal.mFloatVal;
}

const std::string* MLProperty::getStringValue() const
{
	return (mVal.mpStringVal);
}

const MLSignal* MLProperty::getSignalValue() const
{
	return (mVal.mpSignalVal);
}

void MLProperty::setValue(float v)
{
	if(mType == kUndefinedProperty)
		mType = kFloatProperty;
	if(mType == kFloatProperty)
	{
		mVal.mFloatVal = v;
	}
	else
	{
		debug() << "MLProperty::setValue: type mismatch!\n";
	}
}

void MLProperty::setValue(const std::string& v)
{
	if(mType == kUndefinedProperty)
		mType = kStringProperty;
	if(mType == kStringProperty)
	{
		delete mVal.mpStringVal;
		mVal.mpStringVal = new std::string(v);
	}
	else
	{
		debug() << "MLProperty::setValue: type mismatch!\n";
	}
}

void MLProperty::setValue(const MLSignal& v)
{
	if(mType == kUndefinedProperty)
		mType = kSignalProperty;
	if(mType == kSignalProperty)
	{
		delete mVal.mpSignalVal;
		mVal.mpSignalVal = new MLSignal(v);
	}
	else
	{
		debug() << "MLProperty::setValue: type mismatch!\n";
	}
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
			out << *(r.getStringValue());
			break;
		case MLProperty::kSignalProperty:
			out << *(r.getSignalValue());
            break;
	}
	return out;
}

// ----------------------------------------------------------------
// MLModelListener implementation

// called by the Model to notify us that one parameter has changed.
// if the param is new, or the value has changed, we mark the state as changed. 
//
void MLModelListener::propertyChanged(MLSymbol paramSym)
{
	// if the param does not exist in the map yet, this lookup will add it. 
	ModelPropertyState& state = mModelPropertyStates[paramSym];

	// if not initialized, always mark as changed. 
	if(!state.mInitialized)
	{
		state.mChangedSinceUpdate = true;
		state.mInitialized = true;
		return;
	}
	else // check for value change. 
	{
		const MLProperty& modelValue = mpModel->getProperty(paramSym);
		if(modelValue != state.mValue)
		{
			state.mChangedSinceUpdate = true;
		}
	}
}

void MLModelListener::updateChangedProperties() 
{	
	// for all model parameters we know about 
	std::map<MLSymbol, ModelPropertyState>::iterator it;
	for(it = mModelPropertyStates.begin(); it != mModelPropertyStates.end(); it++)
	{
		MLSymbol key = it->first;
		ModelPropertyState& state = it->second;
		
		if(state.mChangedSinceUpdate)
		{
			const MLProperty& modelValue = mpModel->getProperty(key);
			doPropertyChangeAction(key, state.mValue, modelValue);
			state.mChangedSinceUpdate = false;
			state.mValue = modelValue;
		}
	}
}

void MLModelListener::updateAllProperties() 
{	
	mpModel->broadcastAllProperties();
	std::map<MLSymbol, ModelPropertyState>::iterator it;
	for(it = mModelPropertyStates.begin(); it != mModelPropertyStates.end(); it++)
	{
		ModelPropertyState& state = it->second;
		state.mChangedSinceUpdate = true;
	}
	updateChangedProperties();
}

// ----------------------------------------------------------------
// MLModel implementation

MLModel::MLModel()
{
}

MLModel::~MLModel()
{
}

void MLModel::setProperty(MLSymbol p, float v) 
{
	mProperties[p].setValue(v);
	broadcastProperty(p);
}

void MLModel::setProperty(MLSymbol p, const std::string& v) 
{
	mProperties[p].setValue(v);
	broadcastProperty(p);
}

void MLModel::setProperty(MLSymbol p, const MLSignal& v) 
{
	mProperties[p].setValue(v);
	broadcastProperty(p);
}

void MLModel::addPropertyListener(MLModelListener* pL) 
{ 
	mpListeners.push_back(pL); 
}

void MLModel::removePropertyListener(MLModelListener* pToRemove) 
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

void MLModel::broadcastProperty(MLSymbol p) 
{
	std::list<MLModelListener*>::iterator it;
	for(it = mpListeners.begin(); it != mpListeners.end(); it++)
	{
		MLModelListener* pL = *it;
		pL->propertyChanged(p);
	}
}

void MLModel::broadcastAllProperties()
{
	MLPropertyMap::iterator it;
	for(it = mProperties.begin(); it != mProperties.end(); it++)
	{
		MLSymbol p = it->first;
		broadcastProperty(p); 
	}
}

