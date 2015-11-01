
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcSampleRate : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcSampleRate> mInfo;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcSampleRate> classReg("sample_rate");
	// ML_UNUSED MLProcParam<MLProcSampleRate> params[1] = {"in"};	
	// no inputs.
	ML_UNUSED MLProcOutput<MLProcSampleRate> outputs[] = {"out", "inv_out"};
}

// ----------------------------------------------------------------
// implementation

void MLProcSampleRate::process(const int )
{
	MLSignal& y1 = getOutput();
	MLSignal& y2 = getOutput(2);

	y1.setToConstant(getContextSampleRate());
	y2.setToConstant(getContextInvSampleRate());
}



