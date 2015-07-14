
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcFMBandwidth : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcFMBandwidth> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcFMBandwidth> classReg("fm_bandwidth");
	ML_UNUSED MLProcInput<MLProcFMBandwidth> inputs[] = {"c", "m", "i"};
	ML_UNUSED MLProcOutput<MLProcFMBandwidth> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

void MLProcFMBandwidth::process(const int frames)
{
	const MLSignal& c = getInput(1);
	const MLSignal& m = getInput(2);
	const MLSignal& i = getInput(3);
	MLSignal& out = getOutput();
	
	for (int n=0; n<frames; ++n)
	{
        float fc = c[n];
        float fm = m[n];
        float fi = i[n];
//		out[n] = c[n] + m[n]*(i[n] + log(i[n] + 1)); // original formula
		out[n] = fc + fm*(fi + sqrtf(fi*0.5f)*0.5f); // approximation
	}
}