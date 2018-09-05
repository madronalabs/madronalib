
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2018 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <vector>

#include "MLDSP.h"

namespace ml
{

class SignalBuffer
{
public:
	SignalBuffer() {}
	~SignalBuffer() {}
	
	void clear();
	int resize(int length);
	
	int getReadAvailable();
	int getWriteAvailable();

	void write(const float* src, size_t samples);
	void read(float* pDest, size_t samples);

	// TODO, maybe?
//	void write(const DSPVector& src);
//	DSPVector read();
	
	void writeWithOverlapAdd(const float* src, size_t samples, int overlap);
	void readWithOverlap(float* pDest, size_t samples, int overlap);
	
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
	
	size_t advanceDataIndex(size_t start, size_t samples);
	size_t advanceDistanceIndex(size_t start, size_t samples);
	DataRegions getDataRegions(size_t currentIdx, size_t elems, size_t available);
};

}



// TODO multiple vectors as template!

