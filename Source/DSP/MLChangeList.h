
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// RESEARCH
// We are really reinventing wavelets here, in a sense.  So a
// wavelet-based or similar analytic description 
// might be a better, more general, tool for making control signals.

// MLChangeList stores an ordered list of time-stamped changes to a scalar 
// value that can be written out to a 1D time domain signal.
// Changes are relative to the start of a process buffer, and must be added in order. 
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
	void writeToSignal(MLSignal& y, int frames);
	void dump();
	
private:
	void calcGlide();
	inline void setGlideTarget(float target);

	// size of the output vector.
	int mSize;
	// number of changes stored.
	int mChanges;
	
	float mGlideTime;
	int mSampleRate;
	int mGlideTimeInSamples;
	float mInvGlideTimeInSamples;
	int mGlideCounter;
	
	MLSample mValue;
	MLSample mGlideStartValue;
	MLSample mGlideEndValue;
	MLSignal mValueSignal;
	MLSignal mTimeSignal;
	
	bool mDebugTest;
	float mTheta;
};

