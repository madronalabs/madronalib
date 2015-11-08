
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
	void doParams(void);
	
	// coeffs
	MLSample mRate;
	
	// phasor on 0 - (1/ratio), changes at rate of input phasor
	float mOmega;
	
	// x in previous vector
	MLSample mxv;
	
	float mRatio1;
	float mCorrectedRatio;
	float mFilteredRatio;
	
	float mPhaseDiff;
	
	MLBiquad mDxFilter;
	MLBiquad mPhaseFilter;
	MLBiquad mRatioFilter;
	int mDoCorrect1;
	
	
	// temp
	float mpx, mpy;
	
	int mTest;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcRate> classReg("rate");
	//	ML_UNUSED MLProcParam<MLProcRate> params[1] = { "frequency" }; // sync mode?
	ML_UNUSED MLProcInput<MLProcRate> inputs[] = {"in", "ratio"}; 
	ML_UNUSED MLProcOutput<MLProcRate> outputs[] = {"out", "thru"};
}	

// ----------------------------------------------------------------
// implementation


MLProcRate::MLProcRate() :
mRate(1.),
mOmega(0),
mxv(0),
mRatio1(1.f),
mCorrectedRatio(1.f),
mFilteredRatio(1.f),
mDoCorrect1(false),
mTest(0)
{
}

MLProcRate::~MLProcRate()
{
}

void MLProcRate::clear()
{
	mxv = 0.;

	mOmega = 0.;
	
	mDxFilter.clear();
	mPhaseFilter.clear();
	mRatioFilter.clear();
}

void MLProcRate::doParams()
{
	float sr = getContextSampleRate();
	mDxFilter.setSampleRate(sr);
	mDxFilter.setOnePole(100.);
	mPhaseFilter.setSampleRate(sr);
	mPhaseFilter.setOnePole(10.);
	mRatioFilter.setSampleRate(sr);
	mRatioFilter.setOnePole(100.);
	mParamsChanged = false;
}

void MLProcRate::process(const int samples)
{	
	if(mParamsChanged) doParams();
	
	const MLSignal& x = getInput(1);
	const MLSignal& ratio = getInput(2);
	MLSignal& y = getOutput(1);
	MLSignal& thru = getOutput(2);
	
	// temp passthru
	thru = y;
	
	float fratio;
	//float isr = getContextInvSampleRate();
	int maxRatio = 8;
	
	// allow ratio change once per buffer
	fratio = ratio[samples - 1];
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
		
		// set a start seek ratio not quite the target ratio
		// in order to find a lock.
		mRatioFilter.setState(mCorrectedRatio);//*0.9f + mRatio1*0.1f);
		mRatio1 = fratio;
	}
	
	float invRatio = 1.f/mCorrectedRatio;
	
	// get distance from 0 phase within which correction will happen
	//float correctThresh = 1.f / 8.f;//(mCorrectedRatio*4.f); 
	
	// get dx per vector
	float finalX = x[samples - 1];
	float dx = finalX - mxv;		
	mxv = finalX;	
	if(dx < 0.f)
	{
		dx += 1.f;
	}
	
	float dxdt = dx / (float)samples;
	
	if(finalX < 0.f) // input phasor off or just starting 
	{
		if(mOmega > 0.f)
		{
			debug() << "STOP: x : " << finalX << " omega: " << mOmega << " \n";
			x.dump(std::cout, true);
		}
		//mDxFilter.setState(0.f);
		for (int n=0; n<samples; ++n)
		{
			y[n] = -0.001f;
		}
		mPhaseDiff = 0.f;
		mxv = 0.f;
		mOmega = 0.f;
	}
	else 
	{
		//dxdt = dx / (float)samples;

		//dxdt = mDxFilter.processSample(dx / (float)samples);
		
		//debug() << "dxdt: " << dxdt  << " x: " << finalX << "\n";		
		
		bool doCorrect = false;
		float dydt = dxdt;
		
		for (int n=0; n<samples; ++n)
		{
			float px = x[n];
			float py = mOmega;
			
			mpx = px; mpy = py;
			
			// min phase distance from x to 0
			//float dx0 = min(px, (1.f - px));
			//float dy0 = min(py, (1.f - py));
			
			//float thresh = min(0.5f, invRatio);
			doCorrect = true;//(px < correctThresh) && (py < correctThresh );
			
			//keep track of wraps, always correct.
			
			float dxy = 0.f;

			if(doCorrect)
			{
				/*
				if(fabs(pyScaled + 1.0f - px) < fabs(pyScaled - px))
				{
					pDiff = pyScaled + 1.0f - px;
				}
				else
				{
					pDiff = pyScaled - px;
				}
				*/
				
				
				// get minimum x->y distance wrapping on (0, 1)
				float dPlus, dMinus, dPlus2, dMinus2;
				int direction = 0;
				if(py > px)
				{
					direction = 1;
					
					 dPlus = py - px;
					 dMinus = -((1. - py) + px);
					
					if(dPlus < -dMinus)
					{
						dxy = dPlus;
					}
					else
					{
						dxy = dMinus;
					}
				}
				else if(py < px)
				{
					direction = -1;
					
					 dMinus2 = py - px;
					 dPlus2 = ((1. - px) + py);
					
					if(dPlus2 < -dMinus2)
					{
						dxy = dPlus2;
					}
					else
					{
						dxy = dMinus2;
					}				
				}
				
				if(fabs(dxy) > 0.5f)
				{
					debug() << "dxy too big: " << dxy << "  px:" << px << " py: " << py << " invRatio:" << invRatio << "\n";
					//this happened y was 1.99 rescale
				}
				
				mPhaseDiff = dxy;// mPhaseFilter.processSample(dxy);				
				// feedback = mPhaseDiff * 1.f * isr;
				
				// TEMP
				float t = 100.f; 
				//dydt = (t*dxdt - dxy)/t;
				
				// try to intersect x with y in t samples, solve for dydt
				// px + t*dxdt = py + t*dydt;
				
				dydt = (t*dxdt - dxy)/t; 
				
			}
			else
			{
				dydt = dxdt;
			}
			
			// LPF to final seek ratio				
			// mFilteredRatio = mFilteredRatio*(1.f-isr) + mSeekRatio*isr;
			
			mFilteredRatio = mRatioFilter.processSample(dydt);
			
//			float dydt = (mFilteredRatio)*mDxDt;
//			dy -= feedback;
//			mY1 += clamp(dydt, 0.f, 1.f);
			
			mOmega += dydt;
			
			// wrap dx rate phasor
			if(mOmega > invRatio)
			{
				mOmega -= invRatio;
			}		
						
			y[n] = mOmega*mCorrectedRatio;
		}
		
// 		debug() << mY1 << " ";
		// doCorrect = true;
		
//		debug() << ".";
		mTest += samples;
		if(mTest > 10000)
		{
		if(doCorrect && (!mDoCorrect1)) debug() << "-------->\n";
		
		if(doCorrect) debug() << "    dxy: " << std::setprecision(5) << mPhaseDiff << " x: " << mpx << " y: " << mpy << " dy: " << dydt << " dx: " << dxdt << " invratio: " << invRatio << "\n";

		if(!doCorrect && (mDoCorrect1)) debug() << "<--------\n";	
			mTest = 0;

			debug() << x[0] << " / " << y[0] / mCorrectedRatio << "\n";
		}
		
		mDoCorrect1 = doCorrect;
		
	}
}



