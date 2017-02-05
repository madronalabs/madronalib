
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLChangeList.h"

// ----------------------------------------------------------------
// class definition


class MLProcParamToSignal : public MLProc
{
public:
	MLProcParamToSignal();
	~MLProcParamToSignal();

	err resize() override;

	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcParamToSignal> mInfo;
	MLChangeList mChangeList;
	MLSample mVal;	
	float mGlide;
	
	typedef enum 
	{
		kDefault = 0,
		kDecibels = 1,
	}	eLevelMode;	
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcParamToSignal> classReg("param_to_sig");
	ML_UNUSED MLProcParam<MLProcParamToSignal> params[] = {"in", "glide", "level_mode"};
	ML_UNUSED MLProcOutput<MLProcParamToSignal> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation


MLProcParamToSignal::MLProcParamToSignal()
{
//	debug() << "MLProcParamToSignal constructor\n";
	setParam("glide", 0.01f);
	setParam("level_mode", 0);
}


MLProcParamToSignal::~MLProcParamToSignal()
{
//	debug() << "MLProcParamToSignal destructor\n";
}


MLProc::err MLProcParamToSignal::resize()
{
	int vecSize = getContextVectorSize();
	float rate = getContextSampleRate();
	mChangeList.setDims(vecSize);
	mChangeList.setSampleRate(rate);
	mChangeList.setGlideTime(mGlide);

	// TODO exception handling
	return OK;
}

void MLProcParamToSignal::process()
{
	static const ml::Symbol levelModeSym("level_mode");
	static const ml::Symbol glideSym("glide");
	static const ml::Symbol inSym("in");
	
	MLSignal& y = getOutput();
	
	if (mParamsChanged)
	{
		float input = getParam(inSym);
		switch (static_cast<int>(getParam(levelModeSym))) 
		{
			default:
			case kDefault:
				mVal = input;
				break;
			case kDecibels:
				mVal = dBToAmp(input);
				break;
		}
		mChangeList.addChange(mVal, 0);
		mGlide = getParam(glideSym);
		mChangeList.setGlideTime(mGlide);
		mParamsChanged = false;
	}
	
	if (mGlide == 0.f)
	{
		y.setToConstant(mVal);
		// y.setVecToConstant(mVal); // TODO 
	}
	else
	{
		mChangeList.writeToSignal(y, kFloatsPerDSPVector);
	}
}



