
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcPow : public MLProc
{
public:
	MLProcPow();
	~MLProcPow();

	void clear(){};
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcPow> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcPow> classReg("pow");
	ML_UNUSED MLProcInput<MLProcPow> inputs[] = {"base", "exponent" };
	ML_UNUSED MLProcOutput<MLProcPow> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation


MLProcPow::MLProcPow()
{
}


MLProcPow::~MLProcPow()
{
}


void MLProcPow::process(const int frames)
{
	const MLSignal& x1 = getInput(1);
	const MLSignal& x2 = getInput(2);
	MLSignal& y1 = getOutput();
	
	/*
	for (int n=0; n<frames; ++n)
	{
		out[n] = pow(b[n], e[n]);
	}
	*/
	
	const MLSample* px1 = x1.getConstBuffer();
	const MLSample* px2 = x2.getConstBuffer();
	MLSample* py1 = y1.getBuffer();
		
	int c = frames >> kMLSamplesPerSSEVectorBits;
	__m128 vx1, vx2, vr; 	
	
	for (int n = 0; n < c; ++n)
	{
		vx1 = _mm_load_ps(px1);
		vx2 = _mm_load_ps(px2);
		
		vr = exp2Approx4(vx2); // temp calc 2^n
		
		_mm_store_ps(py1, vr);
		px1 += kSSEVecSize;
		px2 += kSSEVecSize;
		py1 += kSSEVecSize;
	}

}





