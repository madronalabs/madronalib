
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLRingBuffer.h"

MLRingBuffer::MLRingBuffer()
{
}

MLRingBuffer::~MLRingBuffer()
{
}

void MLRingBuffer::clear()
{
	PaUtil_FlushRingBuffer(&mBuf);
}

int MLRingBuffer::resize(int length)
{
	int size = 1 << bitsToContain(length);
	mData.setDims(size);
	PaUtil_InitializeRingBuffer( &mBuf, sizeof(MLSample), size, mData.getBuffer() );	
	return mData.getSize();
}

int MLRingBuffer::getRemaining()
{
	return PaUtil_GetRingBufferReadAvailable(&mBuf);
}

int MLRingBuffer::write(const MLSample* pSrc, int samples)
{
	int r = 0;
	if (mData.getSize() >= samples)
	{
		r = PaUtil_WriteRingBuffer( &mBuf, pSrc, samples );
	}
	return r;
}

int MLRingBuffer::writeWithOverlapAdd(const MLSample* pSrc, int samples, int overlap)
{
	int r = 0;
	if (mData.getSize() >= samples)
	{
		r = PaUtil_WriteRingBufferWithOverlapAdd( &mBuf, pSrc, samples, overlap  );
	}
	return r;
}

int MLRingBuffer::read(MLSample* pDest, int samples)
{
	int r = 0;
	if (mData.getSize() >= samples)
	{
		r = PaUtil_ReadRingBuffer( &mBuf, pDest, samples );
	}
	return r;
}

// TODO nicer abstraction for this
int MLRingBuffer::readWithOverlap(MLSample* pDest, int samples, int overlap)
{
	int r = 0;
	if (mData.getSize() >= samples)
	{
		r = PaUtil_ReadRingBufferWithOverlap( &mBuf, pDest, samples, overlap );
	}
	return r;
}



