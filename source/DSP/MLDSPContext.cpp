
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDSPContext.h"


#pragma mark MLDSPContext

MLDSPContext::MLDSPContext()
	: mEnabled(false)
{
	mVectorSize = 0;		
	mSampleRate = kMLToBeCalculated;	
	mInvSampleRate = 1.f;
	mNullInput.setToConstant(0.f);
	mResampleRatio = MLRatio(1, 1);
	mResampleUpOrder = 0;
	mResampleDownOrder = 0;
	// mTestSignal.clear();
}

MLDSPContext::~MLDSPContext()
{
}

int MLDSPContext::setVectorSize(unsigned newSize)
{	
	int retErr = 0;
	mVectorSize = newSize;
	mNullInput.setDims(newSize);
	mNullOutput.setDims(newSize);
	return retErr;
}

void MLDSPContext::setSampleRate(float newRate)
{	
	mSampleRate = newRate;
	mNullInput.setRate(newRate);
	mNullOutput.setRate(newRate);
	mInvSampleRate = 1.0f / mSampleRate;
}

MLSignal& MLDSPContext::getNullInput()
{
	return mNullInput;
}

MLSignal& MLDSPContext::getNullOutput()
{
	return mNullOutput;
}

ml::Time MLDSPContext::getTime() 
{ 
	return mClock.now(); 
}

