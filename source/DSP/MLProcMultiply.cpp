
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcMultiply : public MLProc
{
public:
	void doParams();
	void process(const int n);
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcMultiply> mInfo;
    int mConstantMode;
    const MLSignal* mpX1, *mpX2;
    MLSignal *mpY1;
    const MLSample* mpFX1, *mpFX2;
    MLSample* mpFY1;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcMultiply> classReg("multiply");
	ML_UNUSED MLProcInput<MLProcMultiply> inputs[] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcMultiply> outputs[] = {"out"};
}	


// ----------------------------------------------------------------
// implementation

/*
void MLProcMultiply::process(const int frames)
{
	const MLSignal& x1 = getInput(1);
	const MLSignal& x2 = getInput(2);
	MLSignal& y1 = getOutput();
	for (int n=0; n<frames; ++n)
	{
		y1[n] = x1[n]*x2[n];
	}
}
*/

void MLProcMultiply::doParams()
{
	mpX1 = &getInput(1);
	mpX2 = &getInput(2);
	mpY1 = &(getOutput());

	mpFX1 = mpX1->getConstBuffer();
	mpFX2 = mpX2->getConstBuffer();
	mpFY1 = mpY1->getBuffer();
    
    mParamsChanged = false;
}

void MLProcMultiply::process(const int frames)
{
    if(mParamsChanged) doParams();
	
	const MLSample* px1 = mpFX1;
	const MLSample* px2 = mpFX2;
	MLSample* py1 = mpFY1;

	int c = frames >> kMLSamplesPerSSEVectorBits;
	__m128 vx1, vx2, vr; 	

	for (int n = 0; n < c; ++n)
	{
		vx1 = _mm_load_ps(px1);
		vx2 = _mm_load_ps(px2);
		vr = _mm_mul_ps(vx1, vx2);
		_mm_store_ps(py1, vr);
		px1 += kSSEVecSize;
		px2 += kSSEVecSize;
		py1 += kSSEVecSize;
	}
}
