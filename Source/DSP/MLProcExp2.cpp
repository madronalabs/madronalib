
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcExp2 : public MLProc
{
public:
	MLProcExp2();
	~MLProcExp2();

	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcExp2> mInfo;
	bool mPrecise;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcExp2> classReg("exp2");
	ML_UNUSED MLProcParam<MLProcExp2> params[1] = { "precise" };
	ML_UNUSED MLProcInput<MLProcExp2> inputs[] = { "in" };
	ML_UNUSED MLProcOutput<MLProcExp2> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation


MLProcExp2::MLProcExp2() :
    mPrecise(false)
{
}


MLProcExp2::~MLProcExp2()
{
}

// calculate 2^n for each input sample.
// 
void MLProcExp2::process(const int frames)
{
	static const MLSymbol preciseSym("precise");
	const MLSignal& x1 = getInput(1);
	MLSignal& y1 = getOutput();
	
	if (mParamsChanged) 
	{
		mPrecise = getParam(preciseSym);
		mParamsChanged = false;
	}
	
	if (x1.isConstant())
	{
		y1.setToConstant(pow(2.f, x1[0]));
	}
	else
	{		
		if(mPrecise) // scalar code
		{
			for (int n=0; n<frames; ++n)
			{
				y1[n] = pow(2.f, x1[n]);
			}
		}
		else 
		{
			y1.setConstant(false);
			const MLSample* px1 = x1.getConstBuffer();
			MLSample* py1 = y1.getBuffer();
				
			int c = frames >> kMLSamplesPerSSEVectorBits;
			__m128 vx1, vr; 	
			
			for (int n = 0; n < c; ++n)
			{
				vx1 = _mm_load_ps(px1);		
				vr = exp2Approx4(vx1);
				_mm_store_ps(py1, vr);
				px1 += kSSEVecSize;
				py1 += kSSEVecSize;
			}
		}
	}
}

