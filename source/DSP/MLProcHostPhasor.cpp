
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcHostPhasor.h"

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcHostPhasor> classReg("host_phasor");
	ML_UNUSED MLProcOutput<MLProcHostPhasor> outputs[] = { "out" };
}			

// ----------------------------------------------------------------
// implementation

MLProcHostPhasor::MLProcHostPhasor() :
mDpDt(0),
mPhase1(0.),
mDt(0)
{
	clear();
//	debug() << "MLProcHostPhasor constructor\n";
}

MLProcHostPhasor::~MLProcHostPhasor()
{
//	debug() << "MLProcHostPhasor destructor\n";
}

void MLProcHostPhasor::doParams(void) 
{
	//static const float bpmToHz = 1.f / 240.f * 16.f; // 16th notes
	// mSr = getContextSampleRate();
	
	// new phase
	//float sixteenths = mTime * 4.f;
	//int ks = (int)sixteenths;
	mParamsChanged = false;
}

void MLProcHostPhasor::setTimeAndRate(const double secs, const double ppqPos, const double bpm, bool isPlaying)
{
	// working around a bug I can't reproduce, so I'm covering all the bases.
	if ( ((MLisNaN(ppqPos)) || (MLisInfinite(ppqPos)))
		|| ((MLisNaN(bpm)) || (MLisInfinite(bpm)))
		|| ((MLisNaN(secs)) || (MLisInfinite(secs))) ) 
	{
		debug() << "MLProcHostPhasor::setTimeAndRate: bad input! \n";
		return;
	}
	
	//debug() << "setTimeAndRate: secs " << secs << " ppq: " << ppqPos << " is playing: " << isPlaying << "\n";
	
	double phase = 0.;
	double newTime = clamp(ppqPos, 0., 100000.);
	mActive = (mTime != newTime) && (secs >= 0.) && isPlaying;
	if (mActive)
	{
		mTime = newTime;
		mParamsChanged = true;

		phase = newTime - int(newTime);
		mOmega = (float)phase;
		double newRate = clamp(bpm, 0., 1000.);
		if (mRate != newRate)
		{
			mRate = newRate;				
			mParamsChanged = true;
		}
		
		double dPhase = phase - mPhase1;
		if(dPhase < 0.)
		{
			dPhase += 1.;
		}
		mDpDt = clamp(dPhase/static_cast<double>(mDt), 0., 1.);	
	}
	else
	{
		mOmega = -0.0001f;
		mDpDt = 0.;	
	}
	
	mPhase1 = phase;
	mDt = 0;
}

void MLProcHostPhasor::clear()
{	
	mTime = 0.f;
	mRate = 0.f;
	mOmega = 0.f;
	mActive = 0;
	mPlaying = 0;
}

void MLProcHostPhasor::process(const int samples)
{	
	if (mParamsChanged) 
	{
		doParams();
	}

	MLSignal& y = getOutput();
	for (int n=0; n<samples; ++n)
	{
		mOmega += mDpDt;
		if(mOmega > 1.f) 
		{
			mOmega -= 1.f;
			//debug() << ".";
		}
		y[n] = mOmega;
	}
	mDt += samples;
	//debug() << y[0] << " -- " << y[samples - 1] << "\n";
}
