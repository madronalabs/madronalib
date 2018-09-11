
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2018 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <atomic>
#include <vector>

#include "MLDSP.h"

namespace ml
{
	// SignalBuffer is a single producer, single consumer, lock-free ring buffer for audio.
	// Some nice implementation details come from Portaudio's pa_ringbuffer by Phil Burk and others.
	// C++11 atomics are used to implement the lockfree algorithm.
	
	class SignalBuffer
	{
	public:
		SignalBuffer() {}
		~SignalBuffer() {}
		
		// clear the buffer.
		void clear();
		
		// resize the buffer, allocating 2^n samples sufficient to contain the requested length.
		size_t resize(int length);
		
		// return the number of samples available for reading.
		size_t getReadAvailable();
		
		// return the samples of free space available for writing.
		size_t getWriteAvailable();
		
		// write n samples to the buffer, advancing the write index.
		void write(const float* src, size_t n);
		
		// read n samples from the buffer, advancing the read index.
		void read(float* pDest, size_t n);
		
		// discard n samples by advancing the read index.
		void discard(size_t n);
		
		// add n samples to the buffer and advance the write index by (samples - overlap)
		void writeWithOverlapAdd(const float* src, size_t n, int overlap);
		
		// read n samples from buffer then rewind read point by overlap.
		void readWithOverlap(float* pDest, size_t n, int overlap);
		
		// write a single DSPVectorArray to the buffer, advancing the write index.
		template<int VECTORS>
		void write(const DSPVectorArray<VECTORS>& srcVec)
		{
			constexpr int samples = kFloatsPerDSPVector*VECTORS;
			if(getWriteAvailable() < samples) return;
			
			const auto currentWriteIndex = mWriteIndex.load(std::memory_order_acquire);
			DataRegions dr = getDataRegions(currentWriteIndex, samples);
			
			if(!dr.p2)
			{
				// we have only one region, so we can copy a number of samples known at compile time.
				mWriteIndex.store(advanceDistanceIndex(currentWriteIndex, samples), std::memory_order_release);
				store(srcVec, dr.p1);
			}
			else
			{
				const float* pSrc = srcVec.getConstBuffer();
				std::copy(pSrc, pSrc + dr.size1, dr.p1);
				std::copy(pSrc + dr.size1, pSrc + dr.size1 + dr.size2, dr.p2);
				mWriteIndex.store(advanceDistanceIndex(currentWriteIndex, samples), std::memory_order_release);
			}
		}
		
		// read a single DSPVectorArray from the buffer, advancing the read index.
		template<int VECTORS>
		void read(DSPVectorArray<VECTORS>& destVec)
		{
			constexpr int samples = kFloatsPerDSPVector*VECTORS;
			if(getReadAvailable() < samples) return;
			
			const auto currentReadIndex = mReadIndex.load(std::memory_order_acquire);
			DataRegions dr = getDataRegions(currentReadIndex, samples);
			
			if(!dr.p2)
			{
				// we have only one region, so we can copy a number of samples known at compile time.
				mReadIndex.store(advanceDistanceIndex(currentReadIndex, samples), std::memory_order_release);
				load(destVec, dr.p1);
			}
			else
			{
				float* pDest = destVec.getBuffer();
				std::copy(dr.p1, dr.p1 + dr.size1, pDest);
				std::copy(dr.p2, dr.p2 + dr.size2, pDest + dr.size1);
				mReadIndex.store(advanceDistanceIndex(currentReadIndex, samples), std::memory_order_release);
			}
		}
		
	private:
		std::vector<float> mData;
		float* mDataBuffer;
		size_t mSize{0};
		size_t mDataMask{0};
		size_t mDistanceMask{0};
		
		std::atomic<size_t> mWriteIndex{0};
		std::atomic<size_t> mReadIndex{0};
		
		struct DataRegions
		{
			float* p1;
			size_t size1;
			float* p2;
			size_t size2;
		};
		
		size_t advanceDistanceIndex(size_t start, int samples);
		DataRegions getDataRegions(size_t currentIdx, size_t elems);
	};
}


// TODO try small-local-storage optimization

