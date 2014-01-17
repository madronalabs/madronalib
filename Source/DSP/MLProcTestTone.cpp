
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcTestTone : public MLProc
{
public:
    MLProcTestTone();
	~MLProcTestTone();
	
	void clear();
	void process(const int n);
	MLProcInfoBase& procInfo() { return mInfo; }
    
private:
	MLProcInfo<MLProcTestTone> mInfo;
	MLSample mRootX, mDomain, mRootY, mScale;
	int32_t mOmega32;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcTestTone> classReg("test_tone");
	ML_UNUSED MLProcParam<MLProcTestTone> params[1] = { "mode" }; // todo a few kinds of test signals
	// ML_UNUSED MLProcInput<MLProcTestTone> inputs[] = { };
	ML_UNUSED MLProcOutput<MLProcTestTone> outputs[] = { "out" };
}

// ----------------------------------------------------------------
// implementation

MLProcTestTone::MLProcTestTone()
{
	clear();
	mRootX = sqrtf(2.f);
	const float range = mRootX - mRootX*mRootX*mRootX/6.f;
	mDomain = mRootX * 4.f;
	mScale = 1.f / range;
}

MLProcTestTone::~MLProcTestTone()
{
}

void MLProcTestTone::clear()
{
	mOmega32 = 0.f;
}

// this sine generator makes a looping counter by letting a 32 bit word overflow.
void MLProcTestTone::process(const int samples)
{
	MLSample f;
	MLSignal& y = getOutput();
	
	const float invSr = getContextInvSampleRate();
	static const float intDomain = pow(2.f, 32.f);
	const float srDomain = intDomain * invSr;
	const float domainScale = mDomain / intDomain;
	const float domainOffset = mRootX;
	const float flipOffset = mRootX * 2.f;
	static const float oneSixth = 1.f / 6.f;
	
	float x, fOmega;
	float kFreq = 440.;
    
	for (int n=0; n<samples; ++n)
	{
		f = kFreq;
		
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
