
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PARAMETER_H
#define ML_PARAMETER_H

#include <vector>
#include <list>
#include <utility>

#include "MLDSP.h"
#include "MLSymbol.h"
#include "MLPath.h"
#include "MLSignal.h"

// ----------------------------------------------------------------
#pragma mark Parameter Definitions

typedef enum
{
	kJucePluginParam_Generic,
	kJucePluginParam_Index,
	kJucePluginParam_Seconds,
	kJucePluginParam_Hertz,
	kJucePluginParam_SemiTones,
	kJucePluginParam_Decibels,
	kJucePluginParam_Pan,
	kJucePluginParam_BPM,
}	JucePluginParamUnit;

typedef enum
{
	kJucePluginParam_Linear,
	kJucePluginParam_Exp,
	kJucePluginParam_ExpBipolar
}	JucePluginParamWarpMode;

// ----------------------------------------------------------------
// a published param means: named parameter mParam of mProc is called mAlias.
//
class MLPublishedParam
{
friend class MLProcContainer;
private:
	class ParamAddress
	{
	public:
		ParamAddress(const MLPath & a, const MLSymbol n) : procAddress(a), paramName(n) {}
		~ParamAddress() {}
	
		// procAddress is where to send the param.  can resolve to a single MLProc, 
		// or a list of processors in the case of multiples.  The address is always relative to 
		// the container that publishes the parameters.
		MLPath procAddress;		
		MLSymbol paramName;
	};
	
public:
	
	MLPublishedParam(const MLPath & address, const MLSymbol name, const MLSymbol alias, const MLSymbol type, int idx);
	~MLPublishedParam();
	
	void setRange(MLParamValue low, MLParamValue high, MLParamValue interval, 
		bool log, MLParamValue zt);
	void addAddress(const MLPath & address, const MLSymbol name);

	MLParamValue getValue();
	MLParamValue constrainValue(MLParamValue val);
	MLParamValue getValueAsLinearProportion() const;
	MLParamValue setValueAsLinearProportion (MLParamValue p);
	
//	const MLSignal& getSignalValue() { return mSignalValue; }
//	void setSignalValue(const MLSignal& sig) { mSignalValue = sig; }
	
	unsigned getIndex(void) { return mIndex; }
	MLParamValue getRangeLo(void) const { return mRangeLo; }
	MLParamValue getRangeHi(void) const { return mRangeHi; }
	MLParamValue getInterval(void) { return mInterval; }
	MLParamValue getZeroThresh(void) { return mZeroThreshold; }
	JucePluginParamWarpMode getWarpMode(void) { return mWarpMode; }
	MLParamValue getDefault(void);
	JucePluginParamUnit getUnit(void) { return mUnit; }

	int getGroupIndex(void) { return mGroupIndex; }
	void setGroupIndex(int g) { mGroupIndex = g; }

	MLSymbol getAlias(void) { return mAlias; }
				
	typedef std::list<ParamAddress>::const_iterator AddressIterator;
	AddressIterator beginAddress() { return mAddresses.begin(); }
	AddressIterator endAddress() { return mAddresses.end(); }	

protected:
	void setDefault(MLParamValue val);
	
private:
	std::list<ParamAddress> mAddresses;
	
	// values: TODO use properties of Procs only
	MLParamValue mValue;
	MLSignal mSignalValue;
	
	MLSymbol mAlias;
	MLSymbol mType;
	unsigned mIndex;
	MLParamValue mRangeLo;
	MLParamValue mRangeHi;
	MLParamValue mInterval;
	MLParamValue mZeroThreshold;
	MLParamValue mDefault;
	JucePluginParamUnit mUnit;
	JucePluginParamWarpMode mWarpMode;
	int mGroupIndex;	// -1 for none
};

typedef std::tr1::shared_ptr<MLPublishedParam> MLPublishedParamPtr;

// ----------------------------------------------------------------
#pragma mark named parameter groups

class MLParamGroupMap
{
public:
	MLParamGroupMap();
	~MLParamGroupMap();
	void clear();
	
	// set the current group index to the index matching groupSym. 
	// if an entry for groupSym does not exist, it is made.
	void setGroup(const MLSymbol groupSym);	
	
	// mark the ParamPtr as belonging to the current group.
	void addParamToCurrentGroup(MLPublishedParamPtr p);	
	
	// get the group name of the indexed parameter.
	// const std::string& getGroupName(MLPublishedParamPtr p);
	const std::string& getGroupName(unsigned index);
	
	std::vector<std::string> mGroupVec;
	int mCurrentGroup;
};

#endif // ML_PARAMETER_H