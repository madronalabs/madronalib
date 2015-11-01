
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "xmmintrin.h"

// ----------------------------------------------------------------
// class definition

class MLProcSinePhase : public MLProc
{
public:
	 MLProcSinePhase();
	~MLProcSinePhase();
	
	void clear(){};
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcSinePhase> mInfo;
	MLSample mRootX, mDomain, mRootY, mScale;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcSinePhase> classReg("sine_phase");
	ML_UNUSED MLProcParam<MLProcSinePhase> params[1] = { "gain" };
	ML_UNUSED MLProcInput<MLProcSinePhase> inputs[] = { "phase" }; 
	ML_UNUSED MLProcOutput<MLProcSinePhase> outputs[] = { "out" };
}			


// ----------------------------------------------------------------
// implementation


MLProcSinePhase::MLProcSinePhase()
{
	// for x^3 poly approximation
	mRootX = sqrtf(2.f);
	float range = mRootX - mRootX*mRootX*mRootX/6.f;
	mDomain = mRootX * 4.f;
	mScale = 1.f / range;
}


MLProcSinePhase::~MLProcSinePhase()
{
//	debug() << "MLProcSinePhase destructor\n";
}

void MLProcSinePhase::process(const int samples)
{	
	MLSample g = mScale;
	const MLSignal& phase = getInput(1);
	MLSignal& y = getOutput();
		
	// [0, 1] -> [-sqrt(2), 3*sqrt(2)]
	const float inDomain = 1.f;
	const float domainScale = mDomain / inDomain;
	const float domainOffset = -mRootX;
	const float flipOffset = mRootX * 2.f; 
	const float oneSixth = 1.f / 6.f;
	
	float x, x2, flip, fOmega, fFrac;	
	
	for (int n=0; n<samples; ++n)
	{
		// input on [0, 1] = domain of [-pi, pi] for actual sin fn.
		fOmega = phase[n];
		
		// get fractional part, allowing wrapping
		// NOTE this will fail for inputs less than -32. 
		float fOffset = fOmega + 32.f;
		fFrac = fOffset - (int)fOffset;
		
		flip = fSignBit(fFrac - 0.5f);
		
		// convert to domain of approximation polynomial
		x = fFrac*domainScale + domainOffset;

		// flip upper half of domain
		x2 = x + flip*(flipOffset - x - x);
		
		// sine approx. * gain.
		y[n] = x2*(1 - oneSixth*x2*x2) * g;	// x^3 approx - 3rd harmonic dist. at -40dB
	}	
}
