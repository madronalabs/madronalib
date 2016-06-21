
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcClamp : public MLProc
{
public:
	void process(const int frames) override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcClamp> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcClamp> classReg("clamp");
	ML_UNUSED MLProcParam<MLProcClamp> params[2] = { "min", "max" };
	ML_UNUSED MLProcInput<MLProcClamp> inputs[] = {"in"};	
	ML_UNUSED MLProcOutput<MLProcClamp> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

void MLProcClamp::process(const int frames)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	static ml::Symbol minSym("min");
	static ml::Symbol maxSym("max");
	
	const MLSample fmin = getParam(minSym);
	const MLSample fmax = getParam(maxSym);

	for (int n=0; n < frames; ++n)
	{
		y[n] = ml::clamp(x[n], fmin, fmax);
	}
}



