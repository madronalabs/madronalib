
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

MLProcHostPhasor::MLProcHostPhasor()
{
	clear();
//	debug() << "MLProcHostPhasor constructor\n";
}

MLProcHostPhasor::~MLProcHostPhasor()
{
//	debug() << "MLProcHostPhasor destructor\n";
}

void MLProcHostPhasor::calcCoeffs(void) 
{
	//static const float bpmToHz = 1.f / 240.f * 16.f; // 16th notes
	//const float invSr = getContextInvSampleRate();
	
	// new phase
	//float sixteenths = mTime * 4.f;
	//int ks = (int)sixteenths;
	mParamsChanged = false;
}

void MLProcHostPhasor::setTimeAndRate(const double secs, const double position, const double bpm, bool isPlaying)
{
	double newTime, newRate;

	// working around a bug I can't reproduce, so I'm covering all the bases.
	if ( ((MLisNaN(position)) || (MLisInfinite(position)))
		|| ((MLisNaN(bpm)) || (MLisInfinite(bpm)))
		|| ((MLisNaN(secs)) || (MLisInfinite(secs))) ) 
	{
		debug() << "MLProcHostPhasor::setTimeAndRate: bad input! \n";
		return;
	}
	
	newTime = clamp(position, 0., 100000.);
	mActive = (mTime != newTime);
	if (mActive)
	{
		mTime = newTime;
		mParamsChanged = true;
	}
	
	mPlaying = isPlaying;
	if(mPlaying)
	{	
		if (secs > 0.f) // filter out some Logic weirdness
		{		
			double phase = newTime - int(newTime);
			mOmega = (float)phase;
			newRate = clamp(bpm, 0., 1000.);
			if (mRate != newRate)
			{
				mRate = newRate;				
				mParamsChanged = true;
			}
		}
	}
	else
	{
		mOmega = -0.01f;
	}
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
	MLSignal& y = getOutput();
	
	// coeffs
	if (mParamsChanged) 
	{
		calcCoeffs();
	}
	
	for (int n=0; n<samples; ++n)
	{
		// step output TODO proper ramp
		y[n] = mOmega;
	}
}
