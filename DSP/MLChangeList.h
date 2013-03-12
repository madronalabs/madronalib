
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"


// stores an ordered list of time-stamped changes to a scalar value.
// generates a 1D time series signal.
// changes must be added in order. 
//
class MLChangeList 
{
public:
	MLChangeList();
	~MLChangeList();

	MLProc::err setDims(int size);
	void clearChanges();
	void zero();
	void addChange(MLSample val, int time);
	void setGlideTime(float time);
	void setSampleRate(unsigned rate);
	void writeToSignal(MLSignal& y, unsigned offset, unsigned frames, bool ignoreZeroes = false);
	void dump();
	
private:
	void calcGlide();
	inline void setGlideTarget(float target);

	// size of the output vector.
	unsigned mSize;
	// number of changes stored.
	unsigned mChanges;
	
	float mGlideTime;
	unsigned mSampleRate;
	unsigned mGlideTimeInSamples;
	float mInvGlideTimeInSamples;
	int mGlideCounter;
	
	MLSample temp;
	
	MLSample mValue;
	MLSample mGlideStartValue;
	MLSample mGlideEndValue;
	MLSignal mValueSignal;
	MLSignal mTimeSignal;
};

