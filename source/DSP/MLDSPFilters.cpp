//
//  MLDSPFilters.cpp
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#include "MLDSPFilters.h"

using namespace ml;

FDN::FDN(std::initializer_list<MLPropertyChange> p)
{
	for(auto change : p)
	{
		setProperty(change.mName, change.mValue);
	}
}

// note: order of properties is important! delays property will set
// the number of delays and clear other peoperties.
void FDN::setProperty(Symbol name, MLProperty value)
{
	MLSignal sigVal = value.getSignalValue();
	int currentSize = mDelays.size();
	int newSize = sigVal.getWidth();
	
	if(name == "delays")
	{
		// setting number of delays outside of bounds will turn object into a passthru
		if(!within(newSize, 3, 17))
		{
			newSize = 0;
		}
		
		// resize if needed.
		if(newSize != currentSize)
		{
			mDelays.resize(newSize);
			mFilters.resize(newSize);
			mDelayInputVectors.resize(newSize);
			mFeedbackGains.setDims(newSize);
		}
		
		// set default feedbacks.
		for(int n=0; n<newSize; ++n)
		{
			mFeedbackGains[n] = 1.f;
		}
		
		// set delay times.
		for(int n=0; n<newSize; ++n)
		{
			// we have one DSPVector feedback latency, so delay times can't be smaller than that
			int len = sigVal[n] - kFloatsPerDSPVector;
			len = max(1, len);
			mDelays[n].setDelayInSamples(len);
		}
		clear();
	}
	else if(name == "cutoffs")
	{
		// compute coefficients from cutoffs
		int newValues = min(currentSize, newSize);
		for(int n=0; n<newValues; ++n)
		{
			mFilters[n].setCoeffs(biquadCoeffs::onePole(sigVal[n]));
		}
	}
	else if(name == "gains")
	{
		mFeedbackGains.copy(sigVal);
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

DSPVectorArray<2> FDN::operator()(const DSPVector& input)
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
		DSPVector sumR, sumL;
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
		sumOfDelays *= (2.0f/(float)nDelays);
		
		for(int n=0; n<nDelays; ++n)
		{
			mDelayInputVectors[n] -= (sumOfDelays);
			mDelayInputVectors[n] = mFilters[n](mDelayInputVectors[n]) * mFeedbackGains[n];
			mDelayInputVectors[n] += input;
		}	

		return append(sumL, sumR);
	}
	else
	{
		return repeat<2>(input);
	}
}


