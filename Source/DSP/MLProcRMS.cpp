
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcRMS.h"

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcRMS> classReg("rms");
	ML_UNUSED MLProcParam<MLProcRMS> params[1] = {"time"};
	ML_UNUSED MLProcInput<MLProcRMS> inputs[] = {"in"};
	ML_UNUSED MLProcOutput<MLProcRMS> outputs[] = {"out"}; 
}	// namespace


// ----------------------------------------------------------------
// implementation


MLProcRMS::MLProcRMS()
{
	setParam("time", 0.02f);	// seconds
}


MLProcRMS::~MLProcRMS()
{
//	debug() << "MLProcRMS destructor\n";
}

void MLProcRMS::clear(void) 
{
}

void MLProcRMS::calcCoeffs(void) 
{
	MLSample t = getParam("time");
	MLSample hz = 1.f / t;
	
	mFilter.setSampleRate(getContextSampleRate());
	mFilter.setOnePole(hz);
	
	mParamsChanged = false;

}

void MLProcRMS::process(const int samples)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();

	// coeffs
	if (mParamsChanged) calcCoeffs();
	
	for (int n=0; n<samples; ++n)
	{		
		float xf = x[n];		
		y[n] = mFilter.processSample(xf*xf); // note: no sqrt, not real RMS
	}	
	
	mRMS = y[0]; // klunky
}

