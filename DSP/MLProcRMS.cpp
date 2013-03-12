
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"


class MLProcRMS : public MLProc
{
public:
	MLProcRMS();
	~MLProcRMS();
	
	void clear(void);		
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	void calcCoeffs(void);
	MLProcInfo<MLProcRMS> mInfo;
	
	MLSample mC1;
	MLSample mY1;

};


// ----------------------------------------------------------------
// registry section

namespace{

MLProcRegistryEntry<MLProcRMS> classReg("rms");
ML_UNUSED MLProcParam<MLProcRMS> params[1] = {"time"};
ML_UNUSED MLProcInput<MLProcRMS> inputs[] = {"in"};
ML_UNUSED MLProcOutput<MLProcRMS> outputs[] = {"out"};

}	// namespace


// ----------------------------------------------------------------
// implementation


MLProcRMS::MLProcRMS()
{
	mC1 = 0;
	setParam("time", 0.33f);	// seconds
}


MLProcRMS::~MLProcRMS()
{
//	debug() << "MLProcRMS destructor\n";
}

void MLProcRMS::clear(void) 
{
	mY1 = 0.;
}

void MLProcRMS::calcCoeffs(void) 
{
	MLSample t = getParam("time");
	MLSample hz = 1.f / t;
	mC1 = sinf(kMLTwoPi * hz * getContextInvSampleRate());
	mParamsChanged = false;
debug() << "RMS coeff: " << mC1 << "\n";
}

void MLProcRMS::process(const int samples)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	const MLSample c2 = 1.f - mC1;

	// coeffs
	if (mParamsChanged) calcCoeffs();
	
	for (int n=0; n<samples; ++n)
	{		
		mY1 *= mC1;
		mY1 += c2*x[n]*x[n];		
		y[n] = sqrtf(mY1);
	}	
}

