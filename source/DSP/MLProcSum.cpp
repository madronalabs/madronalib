
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcSum : public MLProc
{
public:
	void process(const int frames);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcSum> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcSum> classReg("sum");
	// no parameters
	ML_UNUSED MLProcInput<MLProcSum> inputs[] = {"*"};	// variable inputs "1", "2", ...
	ML_UNUSED MLProcOutput<MLProcSum> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

void MLProcSum::process(const int frames)
{
	const int inputs = getNumInputs();
	MLSignal& y = getOutput();

	y.clear();
	
	// can optimize in many ways: for small numbers of inputs,
	// loop unrolling, etc.
	for (int i=1; i <= inputs; ++i)
	{
		const MLSignal& xi = getInput(i);
		for (int n=0; n < frames; ++n)
		{
			y[n] += xi[n];
		}
	}
}



