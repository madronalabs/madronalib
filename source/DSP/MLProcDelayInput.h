
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_DELAY_INPUT_H
#define ML_PROC_DELAY_INPUT_H

#include "MLProc.h"
#include "pa_ringbuffer.h"
#include <vector>

// ----------------------------------------------------------------
// class definition


class MLProcDelayInput : public MLProc
{
public:
	MLProcDelayInput();
	~MLProcDelayInput();
	
	err resize();
	void clear();
	void process(const int n);		
	
	int read(MLSample* pOut, int samples);
	int readToOutputSignal(const int samples);
	MLSignal& getBuffer() {return mBuffer;}
	
	MLSignal mBuffer;
	uintptr_t mWriteIndex;
	uintptr_t mLengthMask;

	MLProcInfoBase& procInfo() { return mInfo; }
private:
	MLProcInfo<MLProcDelayInput> mInfo;

	class Tap
	{
	public:
		MLSignal mRingSig;
		PaUtilRingBuffer mRingBuf;
		float mDelayTime;
	};
	
	std::vector<Tap> mTaps;
	
	void doParams(void);	// rebuilds buffer 

};




#endif // ML_PROC_DELAY_INPUT_H
