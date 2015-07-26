
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcFadeBipolar : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
private:
	MLProcInfo<MLProcFadeBipolar> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcFadeBipolar> classReg("fade_bipolar");
	// no parameters.  ML_UNUSED MLProcParam<MLProcFadeBipolar> params[] = { "" };
	ML_UNUSED MLProcInput<MLProcFadeBipolar> inputs[] = {"in1", "in2", "in3", "mix"}; 
	ML_UNUSED MLProcOutput<MLProcFadeBipolar> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

void MLProcFadeBipolar::process(const int frames)
{	
	const MLSignal& in1 = getInput(1);
	const MLSignal& in2 = getInput(2);
	const MLSignal& in3 = getInput(3);
	const MLSignal& mix = getInput(4);
	MLSignal& out = getOutput();
	
	for (int n=0; n<frames; ++n)
	{
		MLSample a = in1[n];
		MLSample b = in2[n];
		MLSample c = in3[n];
		MLSample m = mix[n];
		MLSample absm = fabsf(m);
		MLSample pos = m > 0.;
		MLSample neg = m < 0.;
		MLSample q = pos*c + neg*a;
		out[n] = b + (q - b)*absm;
	}
}
