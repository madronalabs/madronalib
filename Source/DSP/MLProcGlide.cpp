
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcGlide : public MLProc
{
public:
	 MLProcGlide();
	~MLProcGlide();
	
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcGlide> mInfo;
	void calcCoeffs(void);
		
	// coeffs
	MLSample mY1;
	MLSample mEndValue;
	int mRampTimeInSamples;
	MLSample mInvRampTimeInSamples;
	MLSample mStep;
	bool mActive;

};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcGlide> classReg("glide");
	ML_UNUSED MLProcParam<MLProcGlide> params[1] = { "time" };
	ML_UNUSED MLProcInput<MLProcGlide> inputs[] = { "in" }; 
	ML_UNUSED MLProcOutput<MLProcGlide> outputs[] = { "out" };
}	

// ----------------------------------------------------------------
// implementation


MLProcGlide::MLProcGlide()
{
	setParam("time", 1.f);
}


MLProcGlide::~MLProcGlide()
{
}


void MLProcGlide::calcCoeffs(void) 
{
	const float t = getParam("time") + 0.001f;
	const float sr = getContextSampleRate();
	mRampTimeInSamples = (int)(sr*t);
	mInvRampTimeInSamples = 1.f / (float)mRampTimeInSamples;
	mParamsChanged = false;
}

void MLProcGlide::clear()
{
	mY1 = 0.f;
	mStep = 0.f;
	mEndValue = 0.f;
	mActive= false;
}

void MLProcGlide::process(const int samples)
{	
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	float in; 
	float signBeforeAdd, signAfterAdd;
	
	if (mParamsChanged) calcCoeffs();
	
	for (int n=0; n<samples; ++n)
	{
		in = x[n]; // TODO constant input
		
		if (in != mEndValue)
		{
			mEndValue = in;
			mStep = (mEndValue - mY1) * mInvRampTimeInSamples;
			mActive = true;
		}
		
		if(mActive)
		{
			signBeforeAdd = sign(mY1 - mEndValue);		
			mY1 += mStep; 
			signAfterAdd = sign(mY1 - mEndValue);		

			if (signAfterAdd != signBeforeAdd)
			{
				mY1 = mEndValue;
				mActive = false;
			}
		}
		
		y[n] = mY1;
	}
}



   