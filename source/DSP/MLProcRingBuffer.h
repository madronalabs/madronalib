
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_RINGBUFFER_H
#define ML_PROC_RINGBUFFER_H

#include "MLProc.h"
#include "portaudio/pa_ringbuffer.h"

// default size in samples.  should equal kMLSignalViewBufferSize. (MLUI.h)
// But we don't want to refer to UI code here.
const int kMLRingBufferDefaultSize = 128;

enum 
{
	eMLRingBufferNoTrash = 0,
	eMLRingBufferMostRecent = 2
};

// ----------------------------------------------------------------
// class definition


class MLProcRingBuffer : public MLProc
{
public:
	MLProcRingBuffer();
	~MLProcRingBuffer();
	
	void clear() override {}; // TODO should this exist?
	void process() override;		

	// read the buffer contents out to the specified plane of the given signal.
	int readToSignal(MLSignal& outSig, int frames, int plane=0);
	const MLSignal& getOutputSignal();
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	err resize() override; // rebuilds buffer 
	void doParams(void);	
	MLProcInfo<MLProcRingBuffer> mInfo;

	MLSignal mRing;
	MLSignal mTrashSignal;	
	MLSignal mSingleFrameBuffer;
	PaUtilRingBuffer mBuf;
	
	MLSignal test;
	MLSample mTrig1;
};



#endif // ML_PROC_RINGBUFFER_H
