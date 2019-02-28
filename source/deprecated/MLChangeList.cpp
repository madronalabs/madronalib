
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
	mGlideTime = 0.01f;
	mGlideStartValue = mGlideEndValue = 0.f;
	
	mSampleRate = 44100;
	mValue = 0.f;

	calcGlide();
}

MLChangeList::~MLChangeList()
{
}

// mValueSignal and mTimeSignal are able to hold one change per output sample.
// A typical vector might have zero or one changes, but in an extreme case may have
// as many as one per output sample.  And we never want to reallocate it (or anything)
// in the process() method.
void MLChangeList::setDims(int size)
{
	mSize = size;
	MLSample* a = mValueSignal.setDims(size);
	MLSample* b = mTimeSignal.setDims(size);
	if (!a || !b)
	{
		// TODO throw()
	}
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

void MLChangeList::calcGlide()
{
	const unsigned prevGlideTimeInSamples = mGlideTimeInSamples;
	mGlideTimeInSamples = ml::max(1, (int)(mGlideTime * (float)mSampleRate));
	mInvGlideTimeInSamples = 1.f / (float)mGlideTimeInSamples;
	const float glideFrac = (float)mGlideCounter / (float)prevGlideTimeInSamples;
	mGlideCounter = (unsigned)(glideFrac*(float)mGlideTimeInSamples);
}

// add a change: a request to arrive at the given value 
//
// NOTE: not thread safe! current use hopefully ensures correct behavior, but 
// this should be made explicit somehow.
void MLChangeList::addChange(MLSample val, int time)
{
	if (mChanges < mSize)
	{
		/*
		if (mChanges > 0)
		{
			int prevTime = (int)mTimeSignal[mChanges - 1];			
			if (time == prevTime)
			{
				mChanges--;
			}
		}
		 */
		mValueSignal[mChanges] = val;
		mTimeSignal[mChanges] = time;
//	//debug() << "MLChangeList(" << static_cast<void*>(this) << ") add changes:" << mChanges << " time:" << time  << " val:" << val << " glide:" << mGlideTimeInSamples << "\n";
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
  	int size = ml::min(y.getWidth(), mValueSignal.getWidth());
	size = ml::min(size, frames);
	int t=0;
	int changeTime;
	
	// no changes, no glide?  set output signal to constant and bail.
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
				mValue = ml::lerp(mGlideStartValue, mGlideEndValue, x);
			}
			y[t] = mValue;
		}
	}
	else
	{
		// write current value up to each change time, then change current value
		int prevChangeTime = -1;
		float prevTarget = 0.f;
		for(int i = 0; i<mChanges; ++i)
		{
			changeTime = (int)mTimeSignal[i];
			
			if (changeTime >= size)
            {
#ifdef DEBUG
               // //debug() << "warning: MLChangeList time (" << changeTime <<  ") > size!\n";
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
					mValue = ml::lerp(mGlideStartValue, mGlideEndValue, mx);
				}				
				y[t] = mValue;			
			}
			
			float changeTarget = mValueSignal[i];

			// handle multiple changes at same time, a special case for making sure gate signals
			// get retriggered by simultaneous off/on. TODO this smells, redesign some things			
			if(changeTime == prevChangeTime)
			{
				if(changeTime > 0)
				{
					// arrive at first change a sample early
					y[changeTime - 1] = prevTarget;
					y[changeTime] = changeTarget;
					t = changeTime + 1;
				}
				else
				{
					// no room, arrive at most recently added change a sample late
					y[changeTime] = prevTarget;
					y[changeTime + 1] = changeTarget;
					t = changeTime + 2;
				}				
			}

			setGlideTarget(changeTarget);
			prevChangeTime = changeTime;
			prevTarget = changeTarget;
		}
		
		// tick out to end
		for(; t<size; ++t)
		{
			// tick glide
			if (mGlideCounter > 0) 
			{
				mGlideCounter--;
				float mx = (float)(mGlideTimeInSamples - mGlideCounter) * mInvGlideTimeInSamples;
				mValue = ml::lerp(mGlideStartValue, mGlideEndValue, mx);
			}
			y[t] = mValue;
		}		
		mChanges = 0;
	}
}


void MLChangeList::dump(void)
{
//	//debug() << "MLChangeList:   changes " << mChanges << ", c[0] " << mValueSignal[0] << ", counter " << mGlideCounter << ", value " << mValue << "\n";
//	//debug() << "glide time: " << mGlideTimeInSamples << "inv:" << mInvGlideTimeInSamples << "\n";

}


