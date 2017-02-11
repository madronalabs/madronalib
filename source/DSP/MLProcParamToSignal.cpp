
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLChangeList.h"

using namespace ml;
// ----------------------------------------------------------------
// class definition


class MLProcParamToSignal : public MLProc
{
public:
	MLProcParamToSignal();

	err resize() override;
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcParamToSignal> mInfo;
	MLSample mVal;	
	MLChangeList mChangeList;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcParamToSignal> classReg("param_to_sig");
	ML_UNUSED MLProcParam<MLProcParamToSignal> params[] = {"in"};
	ML_UNUSED MLProcOutput<MLProcParamToSignal> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation


MLProcParamToSignal::MLProcParamToSignal()
{

}

MLProc::err MLProcParamToSignal::resize()
{
	float sr = getContextSampleRate();
	mChangeList.setDims(kFloatsPerDSPVector);
	mChangeList.setSampleRate(sr);
	mChangeList.setGlideTime(0.01f);
	return OK;
}

static const ml::Symbol inSym("in");

void MLProcParamToSignal::process()
{
	MLSignal& y = getOutput();
	
	if (mParamsChanged)
	{
		mVal = getParam(inSym);
		mChangeList.addChange(mVal, 0);
		mParamsChanged = false;
	}

	mChangeList.writeToSignal(y, kFloatsPerDSPVector);
}



