
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

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
	void doParams(void);
	
	// coeffs
	MLSample mRate;
	
	// history
	MLSample mX1;
	MLSample mY1;
	
	MLSample mxv;

	float mRatio1;
	float mCorrectedRatio;
	float mFilteredRatio;
	float mSeekRatio;
	
	float mPhaseDiff;
	float mDxDt;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcRate> classReg("rate");
//	ML_UNUSED MLProcParam<MLProcRate> params[1] = { "frequency" }; // sync mode?
	ML_UNUSED MLProcInput<MLProcRate> inputs[] = {"in", "ratio"}; 
	ML_UNUSED MLProcOutput<MLProcRate> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation


MLProcRate::MLProcRate() :
	mRate(1.),
	mX1(0),
	mxv(0),
	mY1(0),
	mRatio1(1.f),
	mCorrectedRatio(1.f),
	mFilteredRatio(1.f),
	mSeekRatio(1.f)
{
}

MLProcRate::~MLProcRate()
{
}

void MLProcRate::clear()
{
	mxv = 0.;
	mX1 = 0.;
	mY1 = 0.;
}

void MLProcRate::process(const int samples)
{	
	const MLSignal& x = getInput(1);
	const MLSignal& ratio = getInput(2);
	MLSignal& y = getOutput();
	float fratio;
	float isr = getContextInvSampleRate();
	int maxRatio = 8;
	float correctThresh = 1.f / (float)maxRatio; 
	
	// allow ratio change once per buffer
	fratio = ratio[0];
	if (fratio != mRatio1)
	{		
		mCorrectedRatio = fratio;	
			
		// correct ratio value		
		bool done = false;
		float rational;
		for(int a=1; a<=maxRatio && !done; ++a)
		{
			for(int b=1; b<=4 && !done; ++b)
			{
				rational = (float)a/(float)b;
				if (fabs(fratio - rational) < 0.001f)
				{
					mCorrectedRatio = rational;
					done = true;	
				}
			}
		}

		// set a final seek ratio not quite the new ratio
		// in order to find a lock.
		mSeekRatio = mCorrectedRatio*0.9f + mRatio1*0.1f;
		mRatio1 = fratio;
	}
	
	// get dx per vector
	float finalX = x[samples - 1];
	float dx = finalX - mxv;		
	mxv = finalX;	
	if(dx < 0.f)
	{
		dx += 1.f;
	}
		
	float dxdt = dx / (float)samples;
	
	if(finalX < 0.f) // input phasor off
	{
		mY1 = -0.01f;
		for (int n=0; n<samples; ++n)
		{
			y[n] = mY1;
		}
		mPhaseDiff = 0.f;
	}
	else 
	{
		float pDiff;
		bool doCorrect = false;
		for (int n=0; n<samples; ++n)
		{
			float px = x[n];
			float py = mY1;
			float pyScaled = py/mCorrectedRatio;
			
			doCorrect = (px < correctThresh) && (py < correctThresh);
			
			float feedback = 0.f;
			if(doCorrect)
			{
				mFilteredRatio = mCorrectedRatio;
				mSeekRatio = mCorrectedRatio;
				
				if(fabs(pyScaled + 1.0f - px) < fabs(pyScaled - px))
				{
					pDiff = pyScaled + 1.0f - px;
				}
				else
				{
					pDiff = pyScaled - px;
				}
				
				// lowpass filter phase difference
				mPhaseDiff = mPhaseDiff*0.99f + pDiff*0.01f;
				feedback = mPhaseDiff * 100.f * isr;
			}
			
			// LPF to final seek ratio				
			mFilteredRatio = mFilteredRatio*(1.f-isr) + mSeekRatio*isr;

			float dy = (mFilteredRatio)*dxdt;
			dy -= feedback;
			mY1 += clamp(dy, 0.f, 1.f);
		
			// wrap
			if(mY1 > 1.f)
			{
				mY1 -= 1.f;
			}		
			
			// if(doCorrect) debug() << "diff: " << std::setprecision(5) << mPhaseDiff << "\n";
			mX1 = px;			
			y[n] = mY1;
		}
	}
}




