
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"

// ----------------------------------------------------------------
// class definition

class MLProcRate : public MLProc
{
public:
	MLProcRate();
	~MLProcRate();
	
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
private:
	MLProcInfo<MLProcRate> mInfo;
	
	// phasor on [0 - 1], changes at rate of input phasor
	float mOmega;
	
	MLSample mx1;	
	float mFloatRatio;
	MLRatio mCorrectedRatio;
	float mRatioNumerator;
	float mRatioNumeratorInv;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcRate> classReg("rate");
	ML_UNUSED MLProcInput<MLProcRate> inputs[] = {"in", "ratio"}; 
	ML_UNUSED MLProcOutput<MLProcRate> outputs[] = {"out", "thru"};
}	

// ----------------------------------------------------------------
// implementation

MLProcRate::MLProcRate() :
	mOmega(0),
	mFloatRatio(1.f),
	mCorrectedRatio(0.f, 0.f),
	mRatioNumerator(1.f),
	mRatioNumeratorInv(1.f)
{
}

MLProcRate::~MLProcRate()
{
}

void MLProcRate::clear()
{
	mOmega = 0.;
}

// if the input is near a low ratio, return the ratio, else return the input unchanged.	
inline MLRatio correctRatio(float rIn)
{
	const int kMaxRatio = 8;
	MLRatio r(0, 0);
	for(int a=1; a<=kMaxRatio; ++a)
	{
		for(int b=1; b<=4; ++b)
		{
			if (fabs(rIn - ((float)a/(float)b)) < 0.001f)
			{
				return MLRatio(a, b);	
			}
		}
	}
	return r;
}

void MLProcRate::process(const int samples)
{	
	const MLSignal& x = getInput(1);
	const MLSignal& ratioSig = getInput(2);
	MLSignal& y = getOutput(1);
	MLSignal& thru = getOutput(2);
	const float isr = getContextInvSampleRate();
	const float kFeedback = isr*10.f; 
	
	// debug passthru
	thru = x;
	
	// allow ratio change once per buffer
	float rIn = ratioSig[samples - 1];
	if (rIn != mFloatRatio)
	{	
		mCorrectedRatio = correctRatio(rIn);
		mFloatRatio = rIn;
		mRatioNumerator = mCorrectedRatio.top;			
		mRatioNumeratorInv = 1.0f/mRatioNumerator;
	}
		
	// if input phasor is off, reset and return.
	if(x[0] < 0.f) 
	{
		mOmega = 0.f;
		y.fill(-0.0001f);
		return;
	}
	
	if(mCorrectedRatio)
	{
		// correct.
		float r = mCorrectedRatio.getFloat();
		float error;
		for (int n=0; n<samples; ++n)
		{
			float px = x[n];
			float dxdt = px - mx1;
			mx1 = px;
			float dydt = max(dxdt*r, 0.f);

			float dxy = px - mOmega/r;
			
			// get modOffset, such that phase difference is 0 at each integer value of modOffset
			float modOffset = dxy*mRatioNumerator;
			
			// get error term, valid at any phase difference.
			error = roundf(modOffset) - modOffset;
			
			// convert back to absolute phase difference
			error *= mRatioNumeratorInv;
			
			// feedback = negative error * time constant
			dydt -= kFeedback*error;
			
			// don't ever run clock backwards.
			dydt = max(dydt, 0.f);
			
			// wrap phasor
			mOmega += dydt;
			if(mOmega >= 1.0f)
			{
				mOmega -= 1.0f;
			}		
						
			y[n] = mOmega;
			
			// temp
			thru[n] = x[n];
		}
	}
	else
	{
		// don't correct, just use float ratio.
		for (int n=0; n<samples; ++n)
		{
			float px = x[n];
			float dxdt = px - mx1;
			mx1 = px;
			float dydt = max(dxdt*mFloatRatio, 0.f);
			
			// wrap phasor
			mOmega += dydt;
			if(mOmega >= 1.0f)
			{
				mOmega -= 1.0f;
			}		
			
			y[n] = mOmega;
		}
	}
}



