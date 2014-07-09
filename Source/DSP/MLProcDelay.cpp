
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcDelay : public MLProc
{
public:
	 MLProcDelay();
	~MLProcDelay();
	
	err resize();
	void clear();
	void process(const int n);		
	
	MLProcInfoBase& procInfo() { return mInfo; }
private:
	MLProcInfo<MLProcDelay> mInfo;
	
	MLSignal mBuffer;
	uintptr_t mWriteIndex;
	uintptr_t mLengthMask;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcDelay> classReg("delay");
	ML_UNUSED MLProcParam<MLProcDelay> params[2] = {"length", "order"};
	ML_UNUSED MLProcInput<MLProcDelay> inputs[] = {"in", "delay"}; 
	ML_UNUSED MLProcOutput<MLProcDelay> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

MLProcDelay::MLProcDelay()
{
	setParam("length", 1.f);
	setParam("order", 0);
}

MLProcDelay::~MLProcDelay()
{
}

MLProc::err MLProcDelay::resize() 
{	
	MLProc::err e = OK;
	const float sr = getContextSampleRate();
	int lenBits = bitsToContain((int)(getParam("length") * sr));
	int length = 1 << lenBits;
	mLengthMask = length - 1;
	
	MLSample* pBuf = mBuffer.setDims(length);
	
	if (!pBuf)
	{
		e = memErr;
	}

	return e;
}

void MLProcDelay::clear() 
{	
	mBuffer.clear();
	mWriteIndex = 0;
}


void MLProcDelay::process(const int frames)
{
	const MLSignal& x = getInput(1);
	const MLSignal& delaySig = getInput(2);
	MLSignal& y = getOutput();
	uintptr_t readIndex;
	const float sr = getContextSampleRate();
	
	for (int n=0; n<frames; ++n)
	{
		mWriteIndex &= mLengthMask;
		mBuffer[mWriteIndex] = x[n];
		
		// read
		// zero order (integer delay)
		MLSample delay = delaySig[n] * sr;
//		int delayInt = floor(delay);
		int delayInt = (int)delay;
		readIndex = mWriteIndex - delayInt;
		readIndex &= mLengthMask;
		y[n] = mBuffer[readIndex];
		mWriteIndex++;
	}
		
	// linear interp:
	//y[n] = frac*x[m+1] + (1-frac)*x[m]
	
	// allpass interp:
	// y[n] = x[m+1] + (1-frac)*x[m] - (1-frac)*y[n-1] 
	
}
	   