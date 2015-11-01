
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcAbs : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcAbs> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcAbs> classReg("abs");
	ML_UNUSED MLProcInput<MLProcAbs> inputs[] = {"in"};
	ML_UNUSED MLProcOutput<MLProcAbs> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

void MLProcAbs::process(const int frames)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	
	for (int n=0; n<frames; ++n)
	{
		y[n] = fabsf(x[n]);
	}
}

//  TODO SSE: fabs == andps with 0x7fffffff 0x7fffffff 0x7fffffff 0x7fffffff

