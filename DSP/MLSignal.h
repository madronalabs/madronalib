
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _MLSIGNAL_H
#define _MLSIGNAL_H

#include <assert.h>
#include <stdint.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include "math.h"
#include "MLDSP.h"
#include "MLVector.h"
#include "MLDebug.h"

#ifdef DEBUG
	const int kMLSignalEndSize = 4;
#else
	const int kMLSignalEndSize = 0;
#endif

extern const MLSample kMLSignalEndSamples[4];

// ----------------------------------------------------------------
// A signal. A finite, discrete representation of data we will 
// generate, modify, look at listen to, etc.
//
// Signals can have multiple dimensions.  If a signal is marked 
// as a time series, the first (most-significant) dimension
// is an index into multiple samples.  Otherwise, the signal has
// the given number of dimensions and no temporal extent.
// 
// 1D matrix: either
// time is dim 1, signal is dimensionless (typical audio signal) 
// or
// no time, signal is 1D on dim[1]
//
// 2D matrix: either 
// time is dim 2, signal is 1D on dims[1] (audio signal in freq. domain)
// or
// no time, signal is 2D on dims[2, 1] (image)
// 
//
// A signal always allocates storage in power of 2 sizes.  For signals of dimension > 1,
// bitmasks are used to force accesses to be within bounds.  

// Signals greater than three dimensions are used so little, it seems to make
// sense for objects that would need those signals to implement them
// as vectors of 3D signals or some such thing.

// Signals can be marked by their creators as constant over the given size.  
// In this case the first data element is the constant value. 
// This allows optimizations to take place downstream, and does not require 
// conditionals in loops to read the signal.


class MLSignal 
{	
public:
	MLSignal();	
	MLSignal(const MLSignal& b);
	MLSignal(int width, int height = 1, int depth = 1); 

	~MLSignal();
	MLSignal & operator= (const MLSignal & other); 

	MLSample* getBuffer (void) const
	{	
		return mDataAligned;
	}

	const MLSample* getConstBuffer (void) const
	{	
		return mDataAligned;
	}
	
	//
	// 1-D access methods 
	//
	
	// inspector, return by value for proc inputs.
	// clients must use const references in order to call this operator.
	// when a signal is marked as constant, mConstantMask = 0 and we return
	// the first value in the array.
	inline MLSample operator[] (int i) const 
	{
		return mDataAligned[i&mConstantMask];
	}

	// mutator, return reference for proc outputs.
	inline MLSample& operator[] (int i)
	{	
		return mDataAligned[i];
	}
	
	// float inspector, return by value 
	inline MLSample getInterpolated(float f) const 
	{
		int i = (int)f;
		float m = f - i;
		return lerp(mDataAligned[i&mConstantMask], mDataAligned[(i+1)&mConstantMask], m);
	}
	
	inline void setToConstant(MLSample k)
	{
		mConstantMask = 0;
		mDataAligned[0] = k;
	}
	
	inline void setConstant(bool k)
	{
		// 1 -> 0
		// 0 -> 0xFFFF
		mConstantMask = ((unsigned long)k - 1);
	}
	
	inline bool isConstant(void) const
	{
		return(mConstantMask == 0);
	}
	
	// 2D access methods
	//
	// mutator, return reference to sample
	// (TODO) does not work with reference?! (MLSignal& t = mySignal; t(2, 3) = k;)
	inline MLSample& operator()(const int i, const int j)
	{
		return mDataAligned[(j<<mWidthBits) + i];
	}
	
	// inspector, return by value
	inline const MLSample operator()(const int i, const int j) const
	{
		return mDataAligned[(j<<mWidthBits) + i];
	}

	const MLSample operator() (const float i, const float j) const;
	const MLSample operator() (const Vec2& pos) const;
	
	// 3D access methods
	//
	// mutator, return sample reference
	inline MLSample& operator()(const int i, const int j, const int k)
	{
		return mDataAligned[(k<<mWidthBits<<mHeightBits) + (j<<mWidthBits) + i];
	}
	
	// inspector, return sample by value
	inline const MLSample operator()(const int i, const int j, const int k) const
	{
		return mDataAligned[(k<<mWidthBits<<mHeightBits) + (j<<mWidthBits) + i];
	}

	const MLSample operator() (const float i, const float j, const float k) const;
	const MLSample operator() (const Vec3 pos) const;
	
	// getFrame() - return const 2D signal made from data in place. 
	const MLSignal getFrame(int i) const;

	// setFrame() - set the 2D frame i to the incoming signal.
	void setFrame(int i, const MLSignal& src);

	// set dims.  return data ptr, or 0 if out of memory.
	MLSample* setDims (int width, int height = 1, int depth = 1);
	
	MLRect getBoundsRect() const { return MLRect(0, 0, mWidth, mHeight); }
	
	int getWidth() const { return mWidth; }
	int getHeight() const { return mHeight; }
	int getDepth() const { return mDepth; }
	int getWidthBits() const { return mWidthBits; }
	int getHeightBits() const { return mHeightBits; }
	int getDepthBits() const { return mDepthBits; }
	int getSize() const { return mSize; }

	int getXStride() const { return (int)sizeof(MLSample); }
	int getYStride() const { return (int)sizeof(MLSample) << mWidthBits; }
	int getZStride() const { return (int)sizeof(MLSample) << mWidthBits << mHeightBits; }
	int getFrames() const;
	
	// rate
	void setRate(MLSampleRate rate){ mRate = rate; }
	MLSampleRate getRate() const { return mRate; }
	
	// I/O
	void read(const MLSample *input, const int offset, const int n);
	void write(MLSample *output, const int offset, const int n);

	void sigClamp(const MLSignal& a, const MLSignal& b);
	void sigMin(const MLSignal& b);
	void sigMax(const MLSignal& b);

	// mix this signal with signal b.
	void sigLerp(const MLSignal& b, const MLSample mix);
	void sigLerp(const MLSignal& b, const MLSignal& mix);

	// binary operators on Signals  TODO rewrite standard
	bool operator==(const MLSignal& b) const;
	bool operator!=(const MLSignal& b) const { return !(operator==(b)); }
	void copy(const MLSignal& b);
	void add(const MLSignal& b);
	void subtract(const MLSignal& b);
	void multiply(const MLSignal& s);	
	void divide(const MLSignal& s);	

	// signal / scalar operators
	void fill(const MLSample f);
	void scale(const MLSample k);	
	void add(const MLSample k);	
	void subtract(const MLSample k);	
	void subtractFrom(const MLSample k);	
	
	// should these be friends?  
	void sigClamp(const MLSample min, const MLSample max);	
	void sigMin(const MLSample min);
	void sigMax(const MLSample max);

	// Convolve the 2D matrix with a radially symmetric 3x3 matrix defined by coefficients
	// kc (center), ke (edge), and kk (corner).
	//
	void convolve3x3r(const MLSample kc, const MLSample ke, const MLSample kk);
	void variance3x3();

	Vec2 correctPeak(const int ix, const int iy) const;

	// unary operators on Signals
	void square();	
	void sqrt();	
	void abs();	
	void inv();	
	void ssign();	
	
	// 2D signal utils
	void makeDuplicateBoundary2D();
	void partialDiffX();
	void partialDiffY();
	// return highest value in signal
	Vec3 findPeak() const;

	void add2D(const MLSignal& b, int destX, int destY);
	void add2D(const MLSignal& b, const Vec2& destOffset);
	
	// transforms
	void clear();
	void invert();

	int checkIntegrity() const;
	float getSum() const;
	float getMean() const;
	float getMin() const;
	float getMax() const;
	void dump(bool verbose = false) const;
	void dump(const MLRect& b) const;
	
	inline bool is1D() const { return((mWidth > 1) && (mHeight == 1) && (mDepth == 1)); }
	inline bool is2D() const { return((mWidth > 1) && (mHeight > 1) && (mDepth == 1)); }
	inline bool is3D() const { return((mWidth > 1) && (mHeight > 1) && (mDepth > 1)); }

private:	
	// private signal constructor: make a reference to a frame of the external signal.
	MLSignal(const MLSignal* other, int frame);

	// handy shorthand for row and plane access
	inline int row(int i) const { return i<<mWidthBits; }
	inline int plane(int i) const { return i<<mWidthBits<<mHeightBits; }

	MLSample* getCopy();

	inline int padSize(int size) { return size + kMLAlignSize - 1 + kMLSignalEndSize; }
	MLSample* allocateData(int size);
	MLSample* initializeData(MLSample* pData, int size);

private:
	// start of data in memory. 
	// If this is 0, we do not own any data.  However, in the case of a
	// reference to another signal mDataAligned may still refer to external data.
	MLSample* mData;
	
	// start of aligned data in memory. 
	MLSample* mDataAligned;
	
	// temporary buffer made if needed for convolution etc.
	MLSample* mCopy;
	MLSample* mCopyAligned;

	// mask for array lookups. By setting to zero, the signal becomes a constant.
	unsigned long mConstantMask;	
	
	// total size in samples, stored for fast access by clear() etc.
	int mSize; 
	
	// store requested size of each dimension. For 1D signals, height is 1.
	int mWidth, mHeight, mDepth; 
	
	// store log2 of actual size of each dimension.
	int mWidthBits, mHeightBits, mDepthBits; 
	
	// Reciprocal of sample rate in Hz.  if negative, signal is not a time series.
	// if zero, rate is a positive one that hasn't been calculated by the DSP engine yet.
	MLSampleRate mRate;
};

typedef std::tr1::shared_ptr<MLSignal> MLSignalPtr;

float rmsDifference2D(const MLSignal& a, const MLSignal& b);
std::ostream& operator<< (std::ostream& out, const MLSignal & r);


#endif // _MLSIGNAL_H