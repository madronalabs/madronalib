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
	// vector size of loop outside the delays. If we are processing sample by sample,
	// this is 1. Delays can't be smaller than the vector size!
	
	int n = min(lengths.getWidth(), (int)mDelays.size());
	
	for(int i=0; i<n; ++i)
	{
		int len = lengths[i] - mVectorSize;
		if(len < 1)
		{
			std::cout << "FDN: requested delay length < 0!\n";
		}
		len = max(2, len);
		mDelays[i].setDelayInSamples(len);
		mDelays[i].clear();
	}
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

MLSignal ml::matrixMultiply2D(MLSignal A, MLSignal B)
{
	if(A.getWidth() != B.getHeight())
	{
		return MLSignal::nullSignal;
	}
	
	int h = A.getHeight();
	int w = B.getWidth();
	int m = A.getWidth();
	MLSignal AB(w, h);
	
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			float ijSum = 0.f;
			for(int k=0; k<m; ++k)
			{
				ijSum += A(k, j)*B(i, k);
			}
			AB(i, j) = ijSum;
		}
	}
	return AB;
}

// NO
MLSignal addScalar(MLSignal a, float x)
{
	int h = a.getHeight();
	int w = a.getWidth();
	
	MLSignal y = a;
	for(int j=0; j<h; ++j)
	{
		for(int i=0; i<w; ++i)
		{
			y(i, j) += x;
		}
	}
	return y;
}

float FDN::processSample(const float x)
{
	int nDelays = mDelays.size();

	// run delays
	for(int n=0; n<nDelays; ++n)
	{
		mDelayInputs[n] = mDelays[n].processSample(mDelayInputs[n]);
	}

	// get output sum
	float outputSum = 0.f;    
	for(int n=0; n<nDelays; ++n)
	{
		outputSum += mDelayInputs[n];
	}
	
	// inputs = input gains*input sample + filters(M*delay outputs)
	// MLSignal feedback = matrixMultiply2D(mFeedbackMatrix, mDelayOutputs);
	float sumOfDelays = 0.f;
	
	// TODO sum operator
	for(int n=0; n<nDelays; ++n)
	{
		sumOfDelays += mDelayInputs[n];
	}
	sumOfDelays *= 2.0f/(float)nDelays;
	
	for(int n=0; n<nDelays; ++n)
	{
		float fx = mDelayInputs[n];
		
		// TODO filter
		mDelayInputs[n] = (fx - sumOfDelays)*0.99f + x;
	}
		
	return outputSum;
}


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
	// MLSignal feedback = matrixMultiply2D(mFeedbackMatrix, mDelayOutputs);
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


