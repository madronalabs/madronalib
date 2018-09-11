
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2018 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSignalBuffer.h"

using namespace ml;

void SignalBuffer::clear()
{
	const auto currentWriteIndex = mWriteIndex.load(std::memory_order_acquire);
	mReadIndex.store(currentWriteIndex, std::memory_order_release);
}

size_t SignalBuffer::resize(int sizeInSamples)
{
	mReadIndex = mWriteIndex = 0;
	
	int sizeBits = ml::bitsToContain(sizeInSamples);
	mSize = std::max(1 << sizeBits, kFloatsPerDSPVector);
	
	try
	{
		mData.resize(mSize);
	}
	catch(const std::bad_alloc& e)
	{
		mDataMask = mDistanceMask = 0;
		return 0;
	}
	
	mDataBuffer = mData.data();
	mDataMask = mSize - 1;
	
	// The distance mask idea is based on code from PortAudio's ringbuffer by Phil Burk.
	// By keeping the read and write pointers constrained to size*2 instead of size, the full state
	// (write - read = size) can be distinguished from the empty state (write - read = 0).
	mDistanceMask = mSize*2 - 1;

	return mSize;
}

size_t SignalBuffer::getReadAvailable()
{
	size_t a = mReadIndex.load(std::memory_order_acquire);
	size_t b = mWriteIndex.load(std::memory_order_relaxed);
	return (b - a)&mDistanceMask;
}

size_t SignalBuffer::getWriteAvailable()
{
	return mSize - getReadAvailable();
}

void SignalBuffer::write(const float* pSrc, size_t samples)
{
	size_t available = getWriteAvailable();
	samples = std::min(samples, available);

	const auto currentWriteIndex = mWriteIndex.load(std::memory_order_acquire);
	DataRegions dr = getDataRegions(currentWriteIndex, samples);
	
	std::copy(pSrc, pSrc + dr.size1, dr.p1);
	if(dr.p2)
	{
		std::copy(pSrc + dr.size1, pSrc + dr.size1 + dr.size2, dr.p2);
	}

	mWriteIndex.store(advanceDistanceIndex(currentWriteIndex, samples), std::memory_order_release);
}

void SignalBuffer::read(float* pDest, size_t samples)
{
	size_t available = getReadAvailable();
	samples = std::min(samples, available);
	
	const auto currentReadIndex = mReadIndex.load(std::memory_order_acquire);
	DataRegions dr = getDataRegions(currentReadIndex, samples);
	
	std::copy(dr.p1, dr.p1 + dr.size1, pDest);
	if(dr.p2)
	{
		std::copy(dr.p2, dr.p2 + dr.size2, pDest + dr.size1);
	}
	
	mReadIndex.store(advanceDistanceIndex(currentReadIndex, samples), std::memory_order_release);
}

void SignalBuffer::discard(size_t samples)
{
	size_t available = getReadAvailable();
	samples = std::min(samples, available);
	const auto currentReadIndex = mReadIndex.load(std::memory_order_acquire);
	mReadIndex.store(advanceDistanceIndex(currentReadIndex, samples), std::memory_order_release);
}

void addSamples(const float * pSrcStart, const float* pSrcEnd, float* pDest)
{
	for(const float* p = pSrcStart; p < pSrcEnd; ++p)
	{
		*pDest++ += *p;
	}
}

void SignalBuffer::writeWithOverlapAdd(const float* pSrc, size_t samples, int overlap)
{
	size_t available = getWriteAvailable();
	
	int samplesRequired = samples*2 - overlap;
	
	// don't write partial windows.
	if(available < samplesRequired) return;

	size_t currentWriteIndex = mWriteIndex.load(std::memory_order_acquire);
	
	// add samples to data in buffer
	DataRegions dr = getDataRegions(currentWriteIndex, samples);
	addSamples(pSrc, pSrc + dr.size1, dr.p1);
	if(dr.p2)
	{
		addSamples(pSrc + dr.size1, pSrc + dr.size1 + dr.size2, dr.p2);
	}
	
	// clear samples for next overlapped add
	currentWriteIndex = advanceDistanceIndex(currentWriteIndex, samples);
	int samplesToClear = samples - overlap;
	dr = getDataRegions(currentWriteIndex, samplesToClear);

	std::fill(dr.p1, dr.p1 + dr.size1, 0.f);
	if(dr.p2)
	{
		std::fill(dr.p2, dr.p2 + dr.size2, 0.f);
	}
	
	currentWriteIndex = advanceDistanceIndex(currentWriteIndex, -overlap);

	mWriteIndex.store(currentWriteIndex, std::memory_order_release);
}

void SignalBuffer::readWithOverlap(float* pDest, size_t samples, int overlap)
{
	size_t available = getReadAvailable() + overlap;
	samples = std::min(samples, available);

	const auto currentReadIndex = mReadIndex.load(std::memory_order_acquire);
	DataRegions dr = getDataRegions(currentReadIndex, samples);
	
	std::copy(dr.p1, dr.p1 + dr.size1, pDest);
	if(dr.p2)
	{
		std::copy(dr.p2, dr.p2 + dr.size2, pDest + dr.size1);
	}
	
	mReadIndex.store(advanceDistanceIndex(currentReadIndex, samples - overlap), std::memory_order_release);
}

size_t SignalBuffer::advanceDistanceIndex(size_t start, int samples)
{
	return (start + samples) & mDistanceMask;
}

SignalBuffer::DataRegions SignalBuffer::getDataRegions(size_t currentIdx, size_t elems)
{
	size_t startIdx = currentIdx & mDataMask;
	if(startIdx + elems > mSize)
	{
		size_t firstHalf = mSize - startIdx;
		size_t secondHalf = elems - firstHalf;
		return DataRegions{mDataBuffer + startIdx, firstHalf,
			mDataBuffer, secondHalf};
	}
	else
	{
		return DataRegions{mDataBuffer + startIdx, elems,
			nullptr, 0};
	}
}
