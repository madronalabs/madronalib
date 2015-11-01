
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcCubicDistort : public MLProc
{
public:
	void process(const int frames);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcCubicDistort> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcCubicDistort> classReg("cubic_distort");
	ML_UNUSED MLProcInput<MLProcCubicDistort> inputs[] = {"in", "distort" };	
	ML_UNUSED MLProcOutput<MLProcCubicDistort> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

void MLProcCubicDistort::process(const int frames)
{
	const MLSignal& x = getInput(1);
	const MLSignal& d = getInput(2);
	MLSignal& y = getOutput();
	
	for (int n=0; n < frames; ++n)
	{
		MLSample in = x[n];
		y[n] = lerp(in, (0.5f*in)*(3.0f - in*in), d[n]);
	}
}



