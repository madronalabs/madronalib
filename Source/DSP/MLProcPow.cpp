
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcPow : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcPow> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcPow> classReg("pow");
	ML_UNUSED MLProcInput<MLProcPow> inputs[] = {"base", "exponent" };
	ML_UNUSED MLProcOutput<MLProcPow> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

void MLProcPow::process(const int frames)
{
	const MLSignal& base = getInput(1);
	const MLSignal& exp = getInput(2);
	MLSignal& out = getOutput();
	
	for (int n=0; n<frames; ++n)
	{
		out[n] = powf(base[n], exp[n]);
	}
}





