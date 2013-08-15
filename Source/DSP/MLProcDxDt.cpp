
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcDxDt : public MLProc
{
public:
	
	void clear();		
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcDxDt> mInfo;
	MLSample xn1;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcDxDt> classReg("dxdt");
	ML_UNUSED MLProcInput<MLProcDxDt> inputs[] = {"in"}; 
	ML_UNUSED MLProcOutput<MLProcDxDt> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

void MLProcDxDt::clear()
{
	xn1 = 0.;
}

void MLProcDxDt::process(const int frames)
{	
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	MLSample fs;

	for (int n = 0; n < frames; ++n)
	{
		fs = (x[n] * getContextSampleRate());
		y[n] = fs - xn1;
		xn1 = fs;
	}
}



   