

// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"
#include "MLProjection.h"

// ----------------------------------------------------------------
// class definition

class MLProcStereoPan : public MLProc
{
public:
	MLProc::err resize() override;
	void process(const int frames) override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcStereoPan> mInfo;
	void calcCoeffs(void);
    MLBiquad mSlewLimiter;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcStereoPan> classReg("stereo_pan");
	// no parameters, all signals. 
	ML_UNUSED MLProcInput<MLProcStereoPan> inputs[] = {"in_l", "in_r", "pan"};
	ML_UNUSED MLProcOutput<MLProcStereoPan> outputs[] = {"out_l", "out_r"};
}

// ----------------------------------------------------------------
// implementation

MLProc::err MLProcStereoPan::resize() 
{
	int sr = getContextSampleRate();
	mSlewLimiter.setSampleRate(sr);
	mSlewLimiter.setOnePole(500);
	return MLProc::OK;
}

void MLProcStereoPan::process(const int samples)
{	
	const MLSignal& inL = getInput(1);
	const MLSignal& inR = getInput(2);
	const MLSignal& pan = getInput(3);
	MLSignal& outL = getOutput(1);
	MLSignal& outR = getOutput(2);
	
	ml::TableProjection LToLGain {0, 1, 0};
	ml::TableProjection LToRGain {0, 0, 1};
	ml::TableProjection RToLGain {1, 0, 0};
	ml::TableProjection RToRGain {0, 1, 0};
    
	for (int n=0; n<samples; ++n)
	{
        float pos = mSlewLimiter.processSample(clamp(pan[n], -1.f, 1.f));
		pos = pos*0.5f + 0.5f; // [-1, 1] -> [0, 1]
		
		outL[n] = inL[n]*LToLGain(pos) + inR[n]*RToLGain(pos);
		outR[n] = inL[n]*LToRGain(pos) + inR[n]*RToRGain(pos);
	}
}
