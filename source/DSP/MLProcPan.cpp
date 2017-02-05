

// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPDeprecated.h"

// ----------------------------------------------------------------
// class definition

class MLProcPan : public MLProc
{
public:
	MLProc::err resize() override;
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcPan> mInfo;
	void calcCoeffs(void);
    MLBiquad mSlewLimiter;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcPan> classReg("pan");
	// no parameters, all signals. 
	ML_UNUSED MLProcInput<MLProcPan> inputs[] = {"in", "pan"}; 
	ML_UNUSED MLProcOutput<MLProcPan> outputs[] = {"out_l", "out_r"};
}

// ----------------------------------------------------------------
// implementation

MLProc::err MLProcPan::resize() 
{
    int sr = getContextSampleRate();
    mSlewLimiter.setSampleRate(sr);
    mSlewLimiter.setOnePole(500);
	return MLProc::OK;
}

void MLProcPan::process()
{	
	const MLSignal& x = getInput(1);
	const MLSignal& pan = getInput(2);
	MLSignal& out1 = getOutput();
	MLSignal& out2 = getOutput(2);
	float in, p, pos;
   
	for (int n=0; n<kFloatsPerDSPVector; ++n)
	{
		in = x[n];
		pos = mSlewLimiter.processSample(ml::clamp(pan[n], -1.f, 1.f));
		pos = pos*0.5f + 0.5f;
		p = in*pos;
		out1[n] = in - p;
		out2[n] = p;
	}
}
