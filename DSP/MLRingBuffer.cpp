
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLRingBuffer.h"

MLRingBuffer::MLRingBuffer() :
	pData(0)
{
}

MLRingBuffer::~MLRingBuffer()
{
	if (pData) delete[] pData;
}

void MLRingBuffer::clear()
{
	PaUtil_FlushRingBuffer(&mBuf);
}

int MLRingBuffer::resize(int length)
{
	int r = 0;
	unsigned size = 1 << bitsToContain(length);
	pData = new MLSample[size];
	
	if (pData)
	{
		r = size;
		PaUtil_InitializeRingBuffer( &mBuf, sizeof(MLSample), size, pData );	
	}
	
	return r;
}

int MLRingBuffer::getRemaining()
{
	return PaUtil_GetRingBufferReadAvailable(&mBuf);
}

int MLRingBuffer::write(MLSample* pOut, unsigned samples) 
{
	int r = 0;
	if (pData)
	{
		r = PaUtil_WriteRingBuffer( &mBuf, pOut, samples );
	}
	return r;
}
		
int MLRingBuffer::read(MLSample* pIn, unsigned samples) 
{
	int r = 0;
	if (pData)
	{
		r = PaUtil_ReadRingBuffer( &mBuf, pIn, samples );
	}
	return r;
}



