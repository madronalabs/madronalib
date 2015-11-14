
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PARAMETER_H
#define ML_PARAMETER_H

#include <vector>
#include <list>

#include "MLDSP.h"
#include "MLSymbol.h"
#include "MLPath.h"
#include "MLSignal.h"
#include "MLProperty.h"
#include "MLRingBuffer.h"

// MLPublishedParam: a parameter of one of the Procs in a DSP graph that is settable through the plugin wrapper.
// There are two concepts wrapped up in this that should be factored out.
// One is a way to view and manipulate a Property. There is a list of these ways. 
// The other is publishing properties of multiple procs in a graph as one settable entity. 
// the Property might be a string, so there is no way to route it to multiple procs within the graph.
// if there were, we could remove the "alsosets" and use proc connections. 
// "alsosets" should check to see if Properties are of compatible types... sort of an ugliness point. 


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
// a published param means: named parameter mParam of mProc is called mPublishedAlias.
//
class MLPublishedParam
{
friend class MLProcContainer;
private:
	class ParamAddress
	{
	public:
		ParamAddress(const MLPath & alias, const MLSymbol name) : procAddress(alias), paramName(name) {}
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
	
	void setRange(MLParamValue low, MLParamValue high, MLParamValue interval, bool log, MLParamValue zt);
	void addAddress(const MLPath & address, const MLSymbol name);

	MLSymbol getType() { return mType; }
	MLParamValue getValue();
	
	const MLProperty& getValueProperty();
	void setValueProperty(const MLProperty& val);
	
	MLParamValue getValueAsLinearProportion() const;
	MLParamValue setValueAsLinearProportion (MLParamValue p);

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

	bool getAutomatable(void);
	void setAutomatable(bool q);
	
	MLSymbol getAlias(void) { return mPublishedAlias; }
				
	typedef std::list<ParamAddress>::const_iterator AddressIterator;
	AddressIterator beginAddress() { return mAddresses.begin(); }
	AddressIterator endAddress() { return mAddresses.end(); }	

protected:
	void setDefault(MLParamValue val);
	
private:
	std::list<ParamAddress> mAddresses;
	MLProperty mParamValue;
	float mTempValue;
	
	MLSymbol mPublishedAlias;
	MLSymbol mType;
	unsigned mIndex;
	MLParamValue mRangeLo;
	MLParamValue mRangeHi;
	MLParamValue mInterval;
	MLParamValue mZeroThreshold;
	MLParamValue mDefault;
	bool mFlip;

	bool mAutomatable;
	JucePluginParamUnit mUnit;
	JucePluginParamWarpMode mWarpMode;
	int mGroupIndex;	// -1 for none
};

typedef std::shared_ptr<MLPublishedParam> MLPublishedParamPtr;

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
	const std::string& getGroupName(unsigned index);
	
	std::vector<std::string> mGroupVec;
	int mCurrentGroup;
};

#endif // ML_PARAMETER_H