
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

	err resize();

	void clear(){};
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcParamToSignal> mInfo;
	MLChangeList mChangeList;
	MLSample mVal;	
	float mGlide;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcParamToSignal> classReg("param_to_sig");
	ML_UNUSED MLProcParam<MLProcParamToSignal> params[] = {"in", "glide"};
	ML_UNUSED MLProcOutput<MLProcParamToSignal> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation


MLProcParamToSignal::MLProcParamToSignal()
{
//	debug() << "MLProcParamToSignal constructor\n";
	setParam("glide", 0.01f);
}


MLProcParamToSignal::~MLProcParamToSignal()
{
//	debug() << "MLProcParamToSignal destructor\n";
}


MLProc::err MLProcParamToSignal::resize()
{
	int vecSize = getContextVectorSize();
	MLSampleRate rate = getContextSampleRate();
	
	mChangeList.setDims(vecSize);
	mChangeList.setSampleRate(rate);
	mChangeList.setGlideTime(mGlide);

	// TODO
	return OK;
}

void MLProcParamToSignal::process(const int frames)
{
	MLSignal& y = getOutput();
	if (mParamsChanged)
	{
		mVal = getParam("in");
		mChangeList.addChange(mVal, 0);
		mGlide = getParam("glide");
		mChangeList.setGlideTime(mGlide);
		mParamsChanged = false;
	}
	
	if (mGlide == 0.f)
	{
		y.setToConstant(mVal);
	}
	else
	{
		mChangeList.writeToSignal(y, frames);
	}
}



