//
//  MLDSPFilters.cpp
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#include "MLDSPFilters.h"

using namespace ml;

void FDN::resize(int n)
{
	mDelays.resize(n);
	for(int i=0; i<n; ++i)
	{
		mDelays[i].setMaxDelayInSamples(mMaxDelayLength);
	}
	
	mFilters.resize(n);
	mDelayInputs.setDims(1, n);
	mFeedbackAmps.setDims(1, n);
	
	mDelayInputVectors.resize(n);
	clear();
}

void FDN::clear()
{
	for(int i=0; i<mDelays.size(); ++i)
	{
		mDelays[i].clear();
	}
	
	for(int i=0; i<mFilters.size(); ++i)
	{
		mFilters[i].clear();
	}
	mDelayInputs.clear();
	for(int i=0; i<mDelayInputVectors.size(); ++i)
	{
		mDelayInputVectors[i] = 0;
	}
}

void FDN::setDelaysInSamples(MLSignal lengths)
{
	int n = min(lengths.getWidth(), (int)mDelays.size());
	
	for(int i=0; i<n; ++i)
	{
		// Delays can't be smaller than the vector size!		
		int len = lengths[i] - kFloatsPerDSPVector;
		if(len < 1)
		{
			std::cout << "FDN: requested delay length < 0!\n";
		}
		len = max(1, len);
		mDelays[i].setDelayInSamples(len);
	}
	clear();
}

/*
void FDN::setLopass(float f)
{
	for(int i=0; i<mFilters.size(); ++i)
	{
		mFilters[i].setCoeffs(ml::biquadCoeffs.onePole(f));
	}
}
*/

DSPVector FDN::operator()(DSPVector x)
{
	int nDelays = mDelays.size();
	
	// run delays, getting DSPVector for each delay 
	for(int n=0; n<nDelays; ++n)
	{
		mDelayInputVectors[n] = mDelays[n](mDelayInputVectors[n]);
	}

	// get output sum
	DSPVector outputSum(0.f);    
	for(int n=0; n<nDelays; ++n)
	{
		outputSum += mDelayInputVectors[n];
	}

	// inputs = input gains*input sample + filters(M*delay outputs)
	DSPVector sumOfDelays(0);	
	for(int n=0; n<nDelays; ++n)
	{
		sumOfDelays += mDelayInputVectors[n];
	}
	sumOfDelays *= (2.0f/(float)nDelays);  // TODO DSPVector *= float 
	
	for(int n=0; n<nDelays; ++n)
	{
		mDelayInputVectors[n] -= sumOfDelays;
		mDelayInputVectors[n] *= 0.99f;
		mDelayInputVectors[n] += x;
	}	

	return outputSum;
}


