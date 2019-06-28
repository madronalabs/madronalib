
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2018 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <atomic>
#include <vector>

#include "MLDSPOps.h"

namespace ml
{
	// DSPBuffer is a single producer, single consumer, lock-free ring buffer for audio.
	// Some nice implementation details are borrowed from Portaudio's pa_ringbuffer by Phil Burk and others.
	// C++11 atomics are used to implement the lockfree algorithm.
	
	class DSPBuffer
	{
	private:
		struct DataRegions
		{
			float* p1;
			size_t size1;
			float* p2;
			size_t size2;
		};

		inline void addSamples(const float * pSrcStart, const float* pSrcEnd, float* pDest)
		{
			for(const float* p = pSrcStart; p < pSrcEnd; ++p)
			{
				*pDest++ += *p;
			}
		}
		
		inline size_t advanceDistanceIndex(size_t start, int samples)
		{
			return (start + samples) & mDistanceMask;
		}
		
		inline DataRegions getDataRegions(size_t currentIdx, size_t elems)
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
		
	public:
		DSPBuffer() {}
		~DSPBuffer() {}
		
		// clear the buffer.
		void clear()
		{
			const auto currentWriteIndex = mWriteIndex.load(std::memory_order_acquire);
			mReadIndex.store(currentWriteIndex, std::memory_order_release);
		}
		
		// resize the buffer, allocating 2^n samples sufficient to contain the requested length.
		size_t resize(int sizeInSamples)
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
			// By keeping the read and write indices constrained to size*2 instead of size, the full state
			// (write - read = size) can be distinguished from the empty state (write - read = 0).
      // getDataRegions() is always used to generate the raw data pointers for reading / writing.
			mDistanceMask = mSize*2 - 1;
			
			return mSize;
		}
		
		// return the number of samples available for reading.
		size_t getReadAvailable()
		{
			size_t a = mReadIndex.load(std::memory_order_acquire);
			size_t b = mWriteIndex.load(std::memory_order_relaxed);
			return (b - a)&mDistanceMask;
		}
		
		// return the samples of free space available for writing.
		size_t getWriteAvailable()
		{
			return mSize - getReadAvailable();
		}
		
		// write n samples to the buffer, advancing the write index.
		void write(const float* pSrc, size_t samples)
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
		
		// read n samples from the buffer, advancing the read index.
		void read(float* pDest, size_t samples)
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
		
		// discard n samples by advancing the read index.
		void discard(size_t samples)
		{
			size_t available = getReadAvailable();
			samples = std::min(samples, available);
			const auto currentReadIndex = mReadIndex.load(std::memory_order_acquire);
			mReadIndex.store(advanceDistanceIndex(currentReadIndex, samples), std::memory_order_release);
		}
		
		// add n samples to the buffer and advance the write index by (samples - overlap)
		void writeWithOverlapAdd(const float* pSrc, size_t samples, int overlap)
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
		
		// read n samples from buffer then rewind read point by overlap.
		void readWithOverlap(float* pDest, size_t samples, int overlap)
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
		
		// read a single DSPVector from the buffer, advancing the read index.
		DSPVector read()
		{
			DSPVector destVec;
			constexpr int samples = kFloatsPerDSPVector;
			if(getReadAvailable() < samples) return DSPVector{};
			
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
			return destVec;
		}

    // write most recent samples from the buffer to the destination without updating the read index.
    void peekMostRecent(float* pDest, int samples)
    {
      int avail = getReadAvailable();
      if(avail < samples) return;

      const auto currentReadIndex = mReadIndex.load(std::memory_order_acquire);
      DataRegions dr = getDataRegions(currentReadIndex, avail);

      if(!dr.p2)
      {
        // we have only one region. copy most recent samples from it.
        // mReadIndex.store(advanceDistanceIndex(currentReadIndex, samples), std::memory_order_release);
        assert(dr.size1 >= samples);
        float* pSrc = dr.p1 + dr.size1 - samples;
        std::copy(pSrc, pSrc + samples, pDest);
      }
      else
      {
        if(dr.size2 >= samples)
        {
          // enough samples are in region 2
          float* pSrc = dr.p2 + dr.size2 - samples;
          std::copy(pSrc, pSrc + samples, pDest);
        }
        else
        {
          // we need samples from both regions.

          // write r1 samples from end
          auto r1Samples = samples - dr.size2;
          float* pSrc1 = dr.p1 + dr.size1 - r1Samples;
          std::copy(pSrc1, pSrc1 + r1Samples, pDest);

          // write all r2 samples after r1 samples
          auto r2Samples = dr.size2;
          float* pSrc2 = dr.p2;
          std::copy(pSrc2, pSrc2 + r2Samples, pDest + r1Samples);
        }
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
	};
} // ml


// TODO try small-local-storage optimization in a production-sized project

