
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcRMS.h"

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcRMS> classReg("rms");
	ML_UNUSED MLProcParam<MLProcRMS> params[1] = {"time"};
	ML_UNUSED MLProcInput<MLProcRMS> inputs[] = {"in"};
	ML_UNUSED MLProcOutput<MLProcRMS> outputs[] = {"out"}; 
}	// namespace


// ----------------------------------------------------------------
// implementation


MLProcRMS::MLProcRMS() : sampleCounter(0)
{
	setParam("time", 0.02f);	// seconds
}


MLProcRMS::~MLProcRMS()
{
//	debug() << "MLProcRMS destructor\n";
}

void MLProcRMS::clear(void) 
{
	mFilter.clear();
}

// volume accessor TODO make generic getter via symbol
float MLProcRMS::getRMS() 
{ 
	//debug() << "RMS: " << mRMS << "\n";
	
	if(mRMS != mRMS)
	{
		debug() << "RMS NaN!\n";
		clear();
	}
	return mRMS; 
}

void MLProcRMS::calcCoeffs(void) 
{
	MLSample t = getParam("time");
	MLSample hz = 1.f / t;
	
	mFilter.clear();
	int sr = getContextSampleRate();
	mFilter.setSampleRate(sr);
	mFilter.setOnePole(hz);
	
	//snapshotSamples = sr / 60;
	
	mParamsChanged = false;
}

void MLProcRMS::process(const int samples)
{
	if (mParamsChanged) calcCoeffs();

	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	
	for (int n=0; n<samples; ++n)
	{		
		float xf = clamp(x[n], -1.f, 1.f);		
		y[n] = mFilter.processSample(xf*xf); // note: no sqrt, not real RMS
	}	

	mRMS = y[0];  // every buffer, kind of arbitrary

	/*
	sampleCounter += samples;
	if(sampleCounter > snapshotSamples)
	{
		mRMS = y[0]; 
		sampleCounter -= snapshotSamples;
	}
	 */
}

