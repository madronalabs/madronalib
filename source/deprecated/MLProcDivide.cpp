
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcDivide : public MLProc
{
public:
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcDivide> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcDivide> classReg("divide");
	ML_UNUSED MLProcInput<MLProcDivide> inputs[] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcDivide> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

void MLProcDivide::process()
{
	const MLSignal& x1 = getInput(1);
	const MLSignal& x2 = getInput(2);
	MLSignal& y1 = getOutput();
	
	const MLSample* px1 = x1.getConstBuffer();
	const MLSample* px2 = x2.getConstBuffer();
	MLSample* py1 = y1.getBuffer();
	
	int c = kSIMDVectorsPerDSPVector; // TODO SIMD
	__m128 vx1, vx2, vr; 	

	for (int n = 0; n < c; ++n)
	{
		vx1 = _mm_load_ps(px1);
		vx2 = _mm_load_ps(px2);
		vr = _mm_div_ps(vx1, vx2);
		_mm_store_ps(py1, vr);
		px1 += kFloatsPerSIMDVector;
		px2 += kFloatsPerSIMDVector;
		py1 += kFloatsPerSIMDVector;
	}
}
