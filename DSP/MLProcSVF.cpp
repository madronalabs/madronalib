
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcSVF : public MLProc
{
public:
	 MLProcSVF();
	~MLProcSVF();
	
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcSVF> mInfo;
	void clear(void);
	MLSample mInState;
	MLSample mLoState;
	MLSample mBandState;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcSVF> classReg("svf");
	ML_UNUSED MLProcInput<MLProcSVF> inputs[] = {"in", "frequency", "q", "mix"}; 
	ML_UNUSED MLProcOutput<MLProcSVF> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation


MLProcSVF::MLProcSVF()
{
}


MLProcSVF::~MLProcSVF()
{
}

void MLProcSVF::clear()
{
	mInState = mLoState = mBandState = 0.f;
}


void MLProcSVF::process(const int samples)
{	
	const MLSignal& x = getInput(1);
	const MLSignal& freq = getInput(2);
	const MLSignal& q = getInput(3);
	const MLSignal& mix = getInput(4);
	MLSignal& y = getOutput();
	
	const float oversample = 1.f / 4.f; 
	const float halfSampleRate = (float)getContextSampleRate()*0.5f;
	float clampedFreq, oneMinusQ, omega, hiState;

	for (int n=0; n<samples; ++n)
	{
		clampedFreq = clamp(freq[n], 1.f, halfSampleRate);
		oneMinusQ = 1.f - q[n];
		omega = 2.0f * fsin1(kMLPi * clampedFreq * getContextInvSampleRate() * oversample);
		
		mInState = x[n];
		
		// regular state variable code here
		mLoState += omega * mBandState;
		hiState = mInState - mLoState - oneMinusQ * mBandState;
		mBandState += omega * hiState;

		mLoState += omega * mBandState;
		hiState = mInState - mLoState - oneMinusQ * mBandState;
		mBandState += omega * hiState;

		mLoState += omega * mBandState;
		hiState = mInState - mLoState - oneMinusQ * mBandState;
		mBandState += omega * hiState;

		mLoState += omega * mBandState;
		hiState = mInState - mLoState - oneMinusQ * mBandState;
		mBandState += omega * hiState;
		
		y[n] = lerpBipolar(mLoState, -hiState, mBandState, mix[n]);
		 // or multiple outs, whatever.  Notch = high + low.
//		y[n] = mLoState;
	}
	
	/*
	// TEMP
	static int test = 0;
	if(test++ > 400)
	
	{
	
		clampedFreq = clamp(freq[0], 1.f, halfSampleRate);

debug() << "svf freq: " << omega << "\n";
debug() << "svf out: " << y[0] << "\n";
		test = 0;
	}
	*/
	
}


