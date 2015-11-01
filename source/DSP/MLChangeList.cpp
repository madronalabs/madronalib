
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLChangeList.h"

MLChangeList::MLChangeList() : mSize(0), mChanges(0), mValue(0.f)
{	
	// setup glide time = 1 sample
	mGlideCounter = 0;
	mGlideTimeInSamples = 1;
	mInvGlideTimeInSamples = 1.f;
	mGlideTime = 0.f;
	mGlideStartValue = mGlideEndValue = 0.f;
	
	mSampleRate = 44100;
	mValue = 0.f;
	
		mDebugTest = false;
		mTheta = 0.;
}

MLChangeList::~MLChangeList()
{
}

// mValueSignal and mTimeSignal are able to hold one change per output sample.
// A typical vector might have zero or one changes, but in an extreme case may have
// as many as one per output sample.  And we never want to reallocate it (or anything)
// in the process() method.
MLProc::err MLChangeList::setDims(int size)
{
	MLProc::err e = MLProc::OK;	
	mSize = size;
	MLSample* a = mValueSignal.setDims(size);
	MLSample* b = mTimeSignal.setDims(size);
	if (!a || !b)
	{
		e = MLProc::memErr;
	}
	return e;
}

void MLChangeList::clearChanges()
{
	mChanges = 0;
}

void MLChangeList::zero()
{
	clearChanges();
	addChange(0.f, 0);
}

void MLChangeList::calcGlide()
{
	const unsigned prevGlideTimeInSamples = mGlideTimeInSamples;
	mGlideTimeInSamples = max(1, (int)(mGlideTime * (float)mSampleRate));
	mInvGlideTimeInSamples = 1.f / (float)mGlideTimeInSamples;
	const float glideFrac = (float)mGlideCounter / (float)prevGlideTimeInSamples;
	mGlideCounter = (unsigned)(glideFrac*(float)mGlideTimeInSamples);
}

void MLChangeList::setGlideTime(float time)
{
	mGlideTime = time;
	calcGlide();
}

void MLChangeList::setSampleRate(unsigned rate)
{
	mSampleRate = rate;
	calcGlide();
}

// add a change: a request to arrive at the given value 
void MLChangeList::addChange(MLSample val, int time)
{
	if (mChanges < mSize)
	{
		if (mChanges > 0)
		{
			int prevTime = (int)mTimeSignal[mChanges - 1];			
			if (time == prevTime)
			{
				mChanges--;
			}
		}
		mValueSignal[mChanges] = val;
		mTimeSignal[mChanges] = time;
//	debug() << "MLChangeList(" << static_cast<void*>(this) << ") add changes:" << mChanges << " time:" << time  << " val:" << val << " glide:" << mGlideTimeInSamples << "\n";
		mChanges++;
	}
}

inline void MLChangeList::setGlideTarget(float target)
{
	mGlideStartValue = mValue;
	mGlideEndValue = target;
	mGlideCounter = mGlideTimeInSamples;
}

// write the input change list from the given offset into the output signal y .
// 
void MLChangeList::writeToSignal(MLSignal& y, int frames)
{
  	int size = min(y.getWidth(), mValueSignal.getWidth());
	size = min(size, frames);
	int t=0;
	int changeTime;
	y.setConstant(false);
	
	// no changes, no glide?  mark constant and bail.
	if (!mChanges && (mGlideCounter <= 0)) 
	{
		y.setToConstant(mValue);		
	}
	else if (!mChanges) // just gliding to target
	{
		for(; t<size; ++t)
		{
			// tick glide
			if (mGlideCounter > 0) 
			{
				mGlideCounter--;
				float x = (float)(mGlideTimeInSamples - mGlideCounter) * mInvGlideTimeInSamples;
				mValue = lerp(mGlideStartValue, mGlideEndValue, x);
			}
			y[t] = mValue;
		}
	}
	else
	{
		y.setConstant(false);
	
		// write current value up to each change time, then change current value
		for(int i = 0; i<mChanges; ++i)
		{
			changeTime = (int)mTimeSignal[i];
			if (changeTime >= size)
            {
#ifdef DEBUG
                debug() << "warning: MLChangeList time (" << changeTime <<  ") > size!\n";
#endif
                
                break;
            }
			
			// write current glide up to change
			for(; t<changeTime; ++t)
			{
				// tick glide
				if (mGlideCounter > 0) 
				{
					mGlideCounter--;
					float mx = (float)(mGlideTimeInSamples - mGlideCounter) * mInvGlideTimeInSamples;
					mValue = lerp(mGlideStartValue, mGlideEndValue, mx);
				}				
				y[t] = mValue;			
			}
			
			float nextTarget = mValueSignal[i];
			setGlideTarget(nextTarget);
		}
		
		// tick out to end
		for(; t<size; ++t)
		{
			// tick glide
			if (mGlideCounter > 0) 
			{
				mGlideCounter--;
				float mx = (float)(mGlideTimeInSamples - mGlideCounter) * mInvGlideTimeInSamples;
				mValue = lerp(mGlideStartValue, mGlideEndValue, mx);
			}
			y[t] = mValue;
		}		
		mChanges = 0;
	}
}

void MLChangeList::dump(void)
{
	debug() << "MLChangeList:   changes " << mChanges << ", c[0] " << mValueSignal[0] << ", counter " << mGlideCounter << ", value " << mValue << "\n";
//	debug() << "glide time: " << mGlideTimeInSamples << "inv:" << mInvGlideTimeInSamples << "\n";


}


