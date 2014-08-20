
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcPhasor : public MLProc
{
public:
	 MLProcPhasor();
	~MLProcPhasor();

	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcPhasor> mInfo;
	void calcCoeffs(void);
	MLSample omega;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcPhasor> classReg("phasor");
	ML_UNUSED MLProcParam<MLProcPhasor> params[] = { "mode" };
	ML_UNUSED MLProcInput<MLProcPhasor> inputs[] = { "frequency", "reset" }; 
	ML_UNUSED MLProcOutput<MLProcPhasor> outputs[] = { "out", "period" };
}			


// ----------------------------------------------------------------
// implementation


MLProcPhasor::MLProcPhasor()
{
//	debug() << "MLProcPhasor constructor\n";
}


MLProcPhasor::~MLProcPhasor()
{
//	debug() << "MLProcPhasor destructor\n";
}

/*
void MLProcPhasor::calcCoeffs(void) 
{
	MLSample f = getParam("freq");
	mCoeffs[0] = kMLTwoPi * f * getInvSampleRate();
	mParamsChanged = false;
}
*/

void MLProcPhasor::clear()
{	
	omega = 0.f;
//	debug() << "phasor clear!~!\n";
}


void MLProcPhasor::process(const int samples)
{	
	const MLSignal& freq = getInput(1);
//	const MLSignal& reset = getInput(2);
	MLSignal& y = getOutput();
	MLSignal& period = getOutput(2);
	float fFreq;
	
	float invSr = getContextInvSampleRate();
	float sr = getContextSampleRate();
	
	// coeffs
//	if (mParamsChanged) calcCoeffs();
//	calcCoeffs();

	// TODO reset

	for (int n=0; n<samples; ++n)
	{
		fFreq = min(freq[n], sr * 0.5f);
		MLSample step = (fFreq) * invSr;
		omega += step;
		if (omega > 1.f)
		{
			omega -= 1.f;
		}
		if (omega < 0.f)
		{
			omega += 1.f;
		}
		y[n] = omega;
		period[n] = 1.f / fFreq; // can approx TODO
	}
}
