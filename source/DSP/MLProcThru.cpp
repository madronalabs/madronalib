
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcThru : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcThru> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcThru> classReg("thru");
	// no parameters, all signals.  ML_UNUSED MLProcParam<MLProcThru> params[1] = { "freq" };
	ML_UNUSED MLProcInput<MLProcThru> inputs[] = {"in"}; 
	ML_UNUSED MLProcOutput<MLProcThru> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

void MLProcThru::process(const int samples)
{	
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	y.copy(x);
}


