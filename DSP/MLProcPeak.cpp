
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcPeak : public MLProc
{
public:
	MLProcPeak();
	~MLProcPeak();
	
	void clear(void);		
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	void calcCoeffs(void);
	MLProcInfo<MLProcPeak> mInfo;
	
	MLSample mY1;
	MLSample mC1;
//	unsigned mTimeInSamples;

	unsigned mTemp;

};


// ----------------------------------------------------------------
// registry section

namespace{

MLProcRegistryEntry<MLProcPeak> classReg("peak");
ML_UNUSED MLProcParam<MLProcPeak> params[1] = {"time"};
ML_UNUSED MLProcInput<MLProcPeak> inputs[] = {"in"};
ML_UNUSED MLProcOutput<MLProcPeak> outputs[] = {"out"};

}	// namespace


// ----------------------------------------------------------------
// implementation


MLProcPeak::MLProcPeak()
{
	mTemp = 0;
	setParam("time", 0.25f);	// seconds
}


MLProcPeak::~MLProcPeak()
{
//	debug() << "MLProcPeak destructor\n";
}

void MLProcPeak::clear(void) 
{
	mY1 = 0.;
	mC1 = 0.;
	calcCoeffs();
}


void MLProcPeak::calcCoeffs(void) 
{
	MLSample t = getParam("time");
	mC1 = kMLTwoPi * getContextInvSampleRate() / t; 
	mParamsChanged = false;
}

// TODO don't calculate so often!  We typically don't use it except in displays.
void MLProcPeak::process(const int samples)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();

	if (mParamsChanged) calcCoeffs();
	
	for (int n=0; n<samples; ++n)
	{	
		MLSample in = x[n];
			
		// TODO branch-free
		if (in > mY1)
		{
			mY1 = in;
		}
		else
		{
			// y = y' + k*dx;
			mY1 -= mC1*mY1;
		}		
		y[n] = mY1;
	}	
}



