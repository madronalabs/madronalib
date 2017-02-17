
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLChangeList.h"
#include "MLDSPPrivate.h"

using namespace ml;

class MLProcParamToSignal : public MLProc
{
public:
	MLProcParamToSignal();

	err resize() override;
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcParamToSignal> mInfo;
	
	LinearGlide mParamToVectorProc;
	DSPVector mParamVector;
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
	// using default glide time
	mParamToVectorProc.setSampleRate(getContextSampleRate());
	return OK;
}

static const ml::Symbol inSym("in");

void MLProcParamToSignal::process()
{
	DSPVector* pOut = reinterpret_cast<DSPVector*>(getOutput(1).getBuffer());
	
	if (mParamsChanged)
	{
		mParamToVectorProc.setInput(getParam(inSym));
		mParamsChanged = false;
	}
	
	*pOut = mParamToVectorProc();
}



