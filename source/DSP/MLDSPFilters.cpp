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

DSPVectorArray<2> FDN::operator()(DSPVector input)
{
	DSPVectorArray<2> output(0.f);    
	int nDelays = mDelays.size();
	if(nDelays > 0)
	{	
		// run delays, getting DSPVector for each delay 
		for(int n=0; n<nDelays; ++n)
		{
			mDelayInputVectors[n] = mDelays[n](mDelayInputVectors[n]);
		}

		// get output sum
		DSPVector sumR(0);
		DSPVector sumL(0);
		for(int n=0; n<(nDelays&(~1)); ++n)
		{
			if(n&1)
			{
				sumL += mDelayInputVectors[n];
			}
			else
			{
				sumR += mDelayInputVectors[n];
			}
		}
		output.setVector<0>(sumL);
		output.setVector<1>(sumR);

		// inputs = input gains*input sample + filters(M*delay outputs)
		// The feedback matrix M is a unit-gain Householder matrix, which is just 
		// the identity matrix minus a constant k, where k = 2/size. Since this can be
		// simplified so much, you just see a few operations here, not a general 
		// matrix multiply.
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
			mDelayInputVectors[n] += input;
		}	

		return output;
	}
	else
	{
		output.fill(input);
		return output;
	}
}


