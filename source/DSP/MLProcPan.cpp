

// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"

// ----------------------------------------------------------------
// class definition

class MLProcPan : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

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

void MLProcPan::calcCoeffs(void) 
{
    int sr = getContextSampleRate();
    mSlewLimiter.setSampleRate(sr);
    mSlewLimiter.setOnePole(500);
	
	mParamsChanged = false;
}

void MLProcPan::process(const int samples)
{	
	const MLSignal& x = getInput(1);
	const MLSignal& pan = getInput(2);
	MLSignal& out1 = getOutput();
	MLSignal& out2 = getOutput(2);
	float in, p, pos;
	const float half = 0.5f;
    
	// coeffs
	if (mParamsChanged) calcCoeffs();
	calcCoeffs();
    
	for (int n=0; n<samples; ++n)
	{
		in = x[n];
        pos = mSlewLimiter.processSample(pan[n]);
		pos = pos * half + half;
		p = in*pos;
		out1[n] = in - p;
		out2[n] = p;
	}
}
