
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcDivide : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

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

void MLProcDivide::process(const int frames)
{
	const MLSignal& x1 = getInput(1);
	const MLSignal& x2 = getInput(2);
	MLSignal& y1 = getOutput();
	
	const bool k1 = x1.isConstant();
	const bool k2 = x2.isConstant();
	const int mode = (k1 << 1) + k2;	
	
	y1.setConstant(false);
	
	const MLSample* px1 = x1.getConstBuffer();
	const MLSample* px2 = x2.getConstBuffer();
	MLSample* py1 = y1.getBuffer();
	
	int c = frames >> kMLSamplesPerSSEVectorBits;
	__m128 vx1, vx2, vr; 	

	switch(mode)
	{
		case 0:
			for (int n = 0; n < c; ++n)
			{
				vx1 = _mm_load_ps(px1);
				vx2 = _mm_load_ps(px2);
				vr = _mm_div_ps(vx1, vx2);
				_mm_store_ps(py1, vr);
				px1 += kSSEVecSize;
				px2 += kSSEVecSize;
				py1 += kSSEVecSize;
			}
		break;
		case 1:
			vx2 = _mm_set1_ps(x2[0]);
			for (int n = 0; n < c; ++n)
			{
				vx1 = _mm_load_ps(px1);
				vr = _mm_div_ps(vx1, vx2);
				_mm_store_ps(py1, vr);
				px1 += kSSEVecSize;
				py1 += kSSEVecSize;
			}
		break;
		case 2:
			vx1 = _mm_set1_ps(x1[0]);
			for (int n = 0; n < c; ++n)
			{
				vx2 = _mm_load_ps(px2);
				vr = _mm_div_ps(vx1, vx2);
				_mm_store_ps(py1, vr);
				px2 += kSSEVecSize;
				py1 += kSSEVecSize;
			}
		break;
		case 3: // yay
			y1.setToConstant(x1[0] / x2[0]);
		break;
	
	
	}
}
