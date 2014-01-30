
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
	void doParams();
	void process(const int n);
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcSineOsc> mInfo;
    static const float kIntDomain, kRootX, kOneSixth, kRange, kDomain, kScale, kDomainScale, kFlipOffset;

	int32_t mOmega32;
    float mInvSrDomain;
};

const float MLProcSineOsc::kIntDomain = powf(2.f, 32.f);
const float MLProcSineOsc::kRootX = sqrtf(2.f);
const float MLProcSineOsc::kOneSixth = 1.f/6.f;
const float MLProcSineOsc::kRange = kRootX - kRootX*kRootX*kRootX*kOneSixth;
const float MLProcSineOsc::kDomain = kRootX*4.f;
const float MLProcSineOsc::kScale = 1.f/kRange;
const float MLProcSineOsc::kDomainScale = kDomain/kIntDomain;
const float MLProcSineOsc::kFlipOffset = kRootX*2.f;


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
}

MLProcSineOsc::~MLProcSineOsc()
{
}

void MLProcSineOsc::clear()
{
	mOmega32 = 0;
}

void MLProcSineOsc::doParams()
{
	const float invSr = getContextInvSampleRate();
	mInvSrDomain = kIntDomain * invSr;
    
    // setup I/O
    
    
    mParamsChanged = false;
}

// this sine generator makes a looping counter by letting a 32 bit word overflow.
void MLProcSineOsc::process(const int samples)
{
	if (mParamsChanged) doParams();
	const MLSignal& freq = getInput(1);
	MLSignal& y = getOutput();

	float f, x, fOmega;
	
	for (int n=0; n<samples; ++n)
	{
		f = freq[n];
		
		// get integer step
		int32_t step32 = (int)(mInvSrDomain * f);
		
		// add increment with wrap
		mOmega32 += step32;
		
		// scale to sin approx domain 
		fOmega = mOmega32 * kDomainScale + kRootX;
		
		// reverse upper half to make triangle wave
		// equivalent to: if (mOmega32 > 0) x = flipOffset - fOmega; else x = fOmega;
		x = fOmega + fSignBit(mOmega32)*(kFlipOffset - fOmega - fOmega);
		
		// sine approx. 
		y[n] = x*(1 - kOneSixth*x*x) * kScale;
	}
}
