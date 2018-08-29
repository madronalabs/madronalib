
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_DELAY_INPUT_H
#define ML_PROC_DELAY_INPUT_H

#include "MLProc.h"
#include "portaudio/pa_ringbuffer.h"
#include <vector>

// ----------------------------------------------------------------
// class definition


class MLProcDelayInput : public MLProc
{
public:
	MLProcDelayInput();
	~MLProcDelayInput();
	
	err resize() override;
	void clear() override;
	void process() override;		
	
	int read(MLSample* pOut, int samples);
	int readToOutputSignal(const int samples);
	MLSignal& getBuffer() {return mBuffer;}
	
	MLSignal mBuffer;
	uintptr_t mWriteIndex;
	uintptr_t mLengthMask;

	MLProcInfoBase& procInfo() override { return mInfo; }
private:
	MLProcInfo<MLProcDelayInput> mInfo;
	
	void doParams(void);	// rebuilds buffer 

};




#endif // ML_PROC_DELAY_INPUT_H
