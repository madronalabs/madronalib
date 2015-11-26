
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// simple wrapper around whatever ringbuffer code we might want to use. 
// right now it's portaudio.

#ifndef _ML_RING_BUFFER_H
#define _ML_RING_BUFFER_H

#include <memory>

#include "pa_ringbuffer.h"
#include "MLDSP.h"

class MLRingBuffer
{
public:
	MLRingBuffer();
	~MLRingBuffer();
	
	void clear();
	int resize(int length);
	int getRemaining();
		
	int write(const MLSample* pSrc, int samples);
	int read(MLSample* pDest, int samples);
	int readWithOverlap(MLSample* pDest, int samples, int overlap);

	PaUtilRingBuffer mBuf;
	float* pData;
};


typedef std::unique_ptr<MLRingBuffer> MLRingBufferPtr;

#endif // _ML_RING_BUFFER_H
