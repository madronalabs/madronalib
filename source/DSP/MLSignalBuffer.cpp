
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

int SignalBuffer::resize(int sizeInSamples)
{
	int sizeBits = ml::bitsToContain(sizeInSamples);
	int size = 1 << sizeBits;
	mSize = std::max(size, kFloatsPerDSPVector);
	
	// TODO error check
	mData.resize(mSize);
	
	mDataBuffer = mData.data();
	mDataMask = mSize - 1;
	mDistanceMask = mSize*2 - 1;

	mReadIndex = 0;
	mWriteIndex = 0;
	return mSize;
}

int SignalBuffer::getReadAvailable()
{
	size_t a = mReadIndex.load(std::memory_order_acquire);
	size_t b = mWriteIndex.load(std::memory_order_relaxed);
	return (b - a)&mDistanceMask;
}

int SignalBuffer::getWriteAvailable()
{
	return mSize - getReadAvailable();
}

void SignalBuffer::write(const float* pSrc, size_t samples)
{
	// debug version check for bad writes
	assert(samples < mSize);
	
	// runtime clamp write size
	size_t available = getWriteAvailable(); // TODO does this need fence?
	
	samples = std::min(samples, available);
	
	const auto currentWriteIndex = mWriteIndex.load(std::memory_order_acquire);

	DataRegions dr = getDataRegions(currentWriteIndex, samples, available);
	
	std::copy(pSrc, pSrc + dr.size1, dr.p1);
	if(dr.p2)
	{
		std::copy(pSrc + dr.size1, pSrc + dr.size1 + dr.size2, dr.p2);
	}

	mWriteIndex.store(advanceDistanceIndex(currentWriteIndex, samples), std::memory_order_release);
}

void SignalBuffer::read(float* pDest, size_t samples)
{
	// debug version check for bad writes
	assert(samples < mSize);
	
	// runtime clamp write size
	size_t available = getReadAvailable(); // TODO does this need fence?
	
	samples = std::min(samples, available);
	
	const auto currentReadIndex = mReadIndex.load(std::memory_order_acquire);
	
	DataRegions dr = getDataRegions(currentReadIndex, samples, available);
	
	std::copy(dr.p1, dr.p1 + dr.size1, pDest);
	if(dr.p2)
	{
		std::copy(dr.p2, dr.p2 + dr.size2, pDest + dr.size1);
	}
	
	mReadIndex.store(advanceDistanceIndex(currentReadIndex, samples), std::memory_order_release);
}

// private

size_t SignalBuffer::advanceDataIndex(size_t start, size_t samples)
{
	return (start + samples) & mDataMask;
}

size_t SignalBuffer::advanceDistanceIndex(size_t start, size_t samples)
{
	return (start + samples) & mDistanceMask;
}

SignalBuffer::DataRegions SignalBuffer::getDataRegions(size_t currentIdx, size_t elems, size_t available)
{
	elems = std::min(elems, available);

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
