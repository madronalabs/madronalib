
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcSineOsc : public MLProc
{
public:
	 MLProcSineOsc();
	~MLProcSineOsc();
	
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcSineOsc> mInfo;
	MLSample mRootX, mDomain, mRootY, mScale;
	int32_t mOmega32;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcSineOsc> classReg("sine_osc");
	ML_UNUSED MLProcParam<MLProcSineOsc> params[1] = { "gain" };
	ML_UNUSED MLProcInput<MLProcSineOsc> inputs[] = { "frequency" }; 
	ML_UNUSED MLProcOutput<MLProcSineOsc> outputs[] = { "out" };
}			

// ----------------------------------------------------------------
// implementation

MLProcSineOsc::MLProcSineOsc()
{
	clear();	
	mRootX = sqrtf(2.f);
	const float range = mRootX - mRootX*mRootX*mRootX/6.f;
	mDomain = mRootX * 4.f;
	mScale = 1.f / range;
}

MLProcSineOsc::~MLProcSineOsc()
{
}

void MLProcSineOsc::clear()
{
	mOmega32 = 0.f;
}

// this sine generator makes a looping counter by letting a 32 bit word overflow.
void MLProcSineOsc::process(const int samples)
{	
	MLSample f;
	const MLSignal& freq = getInput(1);
	MLSignal& y = getOutput();
	
	const float invSr = getContextInvSampleRate();
	static const float intDomain = pow(2.f, 32.f);
	const float srDomain = intDomain * invSr;
	const float domainScale = mDomain / intDomain;
	const float domainOffset = mRootX; 
	const float flipOffset = mRootX * 2.f; 
	static const float oneSixth = 1.f / 6.f;	
	
	float x, fOmega;	
	
	for (int n=0; n<samples; ++n)
	{
		f = freq[n];
		
		// get integer step
		int32_t step32 = (int)(srDomain * f);
		
		// add increment with wrap
		mOmega32 += step32;
		
		// scale to sin approx domain 
		fOmega = mOmega32 * domainScale + domainOffset;
		
		// reverse upper half to make triangle wave
		// equivalent to: if (mOmega32 > 0) x = flipOffset - fOmega; else x = fOmega;
		x = fOmega + fSignBit(mOmega32)*(flipOffset - fOmega - fOmega);
		
		// sine approx. 
		y[n] = x*(1 - oneSixth*x*x) * mScale;	
	}
}
