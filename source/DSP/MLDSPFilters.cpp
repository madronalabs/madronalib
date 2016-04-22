//
//  MLDSPFilters.cpp
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#include "MLDSPFilters.h"

using namespace ml;

void FDN::setDelaysInSamples(MLSignal lengths)
{
	int newSize = lengths.getWidth();
	
	// setting number of delays outside of bounds will turn object into a passthru
	if(!within(newSize, 3, 17))
	{
		newSize = 0;
	}
	
	// resize if needed.
	if(newSize != mDelays.size())
	{
		mDelays.resize(newSize);
		mFilters.resize(newSize);
		mDelayInputVectors.resize(newSize);
		mFeedbackGains.resize(newSize);
	}
	
	// set default feedbacks.
	for(int n=0; n<newSize; ++n)
	{
		mFeedbackGains[n] = 1.f;
	}
	
	// resize delay buffers if needed. 
	for(int n=0; n<newSize; ++n)
	{
		int newLength = lengths[n]; // todo function instead
		int currentLength = mDelays[n].getMaxDelayInSamples();
		if(newLength > currentLength)
		{
			mDelays[n].setMaxDelayInSamples(newLength);
		}
	}
	
	// set delay times.
	for(int n=0; n<newSize; ++n)
	{
		// we have one DSPVector feedback latency, so delay times can't be smaller	
		int len = lengths[n] - kFloatsPerDSPVector;
		len = max(1, len);
		mDelays[n].setDelayInSamples(len);
	}
	clear();
}

void FDN::setFilterCutoffs(MLSignal filterCutoffs)
{
	int newValues = min(filterCutoffs.getWidth(), (int)mFilters.size());
	for(int i=0; i<newValues; ++i)
	{
		mFilters[i].setCoeffs(biquadCoeffs::onePole(filterCutoffs[i]));
	}
}

void FDN::setFeedbackGains(MLSignal gains)
{
	int newValues = min(gains.getWidth(), (int)mFilters.size());
	for(int i=0; i<newValues; ++i)
	{
		mFeedbackGains[i] = gains[i];
	}
}

void FDN::clear()
{
	for(int i=0; i<mDelays.size(); ++i)
	{
		mDelays[i].clear();
		mFilters[i].clear();
		mDelayInputVectors[i] = 0;
	}
}

DSPVector FDN::operator()(DSPVector x)
{
	int nDelays = mDelays.size();
	if(nDelays > 0)
	{	
		// run delays, getting DSPVector for each delay 
		for(int n=0; n<nDelays; ++n)
		{
			mDelayInputVectors[n] = mDelays[n](mDelayInputVectors[n]);
		}

		// get output sum
		// TODO could use 2D DSPVector type of thing instead of std::vector
		// TODO make stereo mix w/ parameters
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
			mDelayInputVectors[n] -= (sumOfDelays);
			mDelayInputVectors[n] = mFilters[n](mDelayInputVectors[n]) * mFeedbackGains[n];
			mDelayInputVectors[n] += x;
		}	

		return outputSum;
	}
	else
	{
		return x;
	}
}


