
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// simple wrapper around whatever ringbuffer code we might want to use. 
// right now it's portaudio.

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
		
	int write(MLSample* pOut, unsigned samples);
	int read(MLSample* pIn, unsigned samples);

	PaUtilRingBuffer mBuf;
	float* pData;
};


typedef std::tr1::shared_ptr<MLRingBuffer> MLRingBufferPtr;