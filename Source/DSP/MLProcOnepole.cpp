
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcOnepole : public MLProc
{
public:
	 MLProcOnepole();
	~MLProcOnepole();
	
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcOnepole> mInfo;
	void doParams(void);
		
	// coeffs
	MLSample mK;
	
	// history
	MLSample mY1;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcOnepole> classReg("onepole");
	ML_UNUSED MLProcParam<MLProcOnepole> params[1] = { "frequency" };
	ML_UNUSED MLProcInput<MLProcOnepole> inputs[] = {"in"}; 
	ML_UNUSED MLProcOutput<MLProcOnepole> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation


MLProcOnepole::MLProcOnepole()
{
	mK = 0.f;
	mY1 = 0.f;
	setParam("frequency", 1000.f);
}

MLProcOnepole::~MLProcOnepole()
{
}

void MLProcOnepole::doParams(void) 
{
	const float f = clamp(getParam("frequency"), 50.f, getContextSampleRate() * 0.25f);
	const float invSr = getContextInvSampleRate();
	mK = fsin1(kMLTwoPi * f * invSr);
	assert(!MLisNaN(mK));
	mParamsChanged = false;
}

void MLProcOnepole::clear()
{
	mY1 = 0.;
}


void MLProcOnepole::process(const int samples)
{	
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	float dxdt; 
	
	if (mParamsChanged) doParams();
	
	for (int n=0; n<samples; ++n)
	{
		dxdt = x[n] - mY1;
		mY1 += mK*dxdt;
		y[n] = mY1;
	}
}



   