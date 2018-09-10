
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2018 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

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
		
		// write a single DSPVector to the buffer, advancing the write index.
		void write(const DSPVector& v);
		
		// read a single DSPVector from the buffer, advancing the read index.
		DSPVector read();
		
		// discard n samples by advancing the read index.
		void discard(size_t n);
		
		// add n samples to the buffer and advance the write index by (samples - overlap)
		void writeWithOverlapAdd(const float* src, size_t n, int overlap);
		
		// read n samples from buffer then rewind read point by overlap.
		void readWithOverlap(float* pDest, size_t n, int overlap);
		
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
		
		size_t advanceDataIndex(size_t start, int samples);
		size_t advanceDistanceIndex(size_t start, int samples);
		DataRegions getDataRegions(size_t currentIdx, size_t elems);
	};
}



// TODO multiple column DSPVectors as template!
// TODO try small-local-storage optimization

