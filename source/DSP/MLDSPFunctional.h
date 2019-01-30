//
// MLDSPFilters.h
// madronalib
//
// Created by Randy Jones on 4/14/2016
//


#pragma once

#include "MLDSPOps.h"

#include <functional>

namespace ml
{
	
	// ----------------------------------------------------------------
	// functional
	
	// Evaluate a function (void)->(float), store at each element of the DSPVectorArray and return the result.
	// x is a dummy argument just used to infer the vector size.
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> map(std::function<float()> f, const DSPVectorArray<VECTORS> x)
	{
		DSPVectorArray<VECTORS> y;
		for(int n=0; n<kFloatsPerDSPVector*VECTORS; ++n)
		{
			y[n] = f();
		}
		return y;
	}
	
	// Apply a function (float)->(float) to each element of the DSPVectorArray x and return the result.
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> map(std::function<float(float)> f, const DSPVectorArray<VECTORS> x)
	{
		DSPVectorArray<VECTORS> y;
		for(int n=0; n<kFloatsPerDSPVector*VECTORS; ++n)
		{
			y[n] = f(x[n]);
		}
		return y;
	}
	
	// Apply a function (DSPVector, int row)->(DSPVector) to each row of the DSPVectorArray x and return the result.
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> map(std::function<DSPVector(const DSPVector&)> f, const DSPVectorArray<VECTORS> x)
	{
		DSPVectorArray<VECTORS> y;
		for(int j=0; j<VECTORS; ++j)
		{
			y.setRowVectorUnchecked(j, f(x.getRowVectorUnchecked(j)));
		}
		return y;
	}
	
	// Apply a function (DSPVector, int row)->(DSPVector) to each row of the DSPVectorArray x and return the result.
	template<int VECTORS>
	inline DSPVectorArray<VECTORS> map(std::function<DSPVector(const DSPVector&, int)> f, const DSPVectorArray<VECTORS> x)
	{
		DSPVectorArray<VECTORS> y;
		for(int j=0; j<VECTORS; ++j)
		{
			y.setRowVectorUnchecked(j, f(x.getRowVectorUnchecked(j), j));
		}
		return y;
	}
	
	// Here are some function objects that take DSP functions as parameters to operator() and apply
	// the function in a different context, such as upsampled, overlap-added or in the frequency domain.

	// TODO improve pack / unpack of rows for sending multiple sources to functions

	// Upsample2x is a function object that given a process function f, 
	// upsamples the input x by 2, applies f, downsamples and returns the result.
	// the total delay from the resampling filters used is about 3 samples.	
	
	template<int IN_ROWS, int OUT_ROWS>
	class Upsample2x
	{
		typedef std::function<DSPVectorArray<OUT_ROWS>(const DSPVectorArray<IN_ROWS>)> ProcessFn;
		
	public:		
		// operator() takes two arguments: a process function and an input DSPVectorArray. 
		// The optional argument DSPVectorArray<0>() allows passing only one argument in the case of a generator with 0 input rows.
		inline DSPVectorArray<OUT_ROWS> operator()(ProcessFn fn, const DSPVectorArray<IN_ROWS> vx = DSPVectorArray<0>())
		{
			// upsample each row of input to 2x buffers			
			for(int j=0; j < IN_ROWS; ++j)
			{
				DSPVector x1a = mUppers[j].upsampleFirstHalf(vx.constRow(j));				
				DSPVector x1b = mUppers[j].upsampleSecondHalf(vx.constRow(j));
				mUpsampledInput1.row(j) = x1a;
				mUpsampledInput2.row(j) = x1b;
			}
			
			// process upsampled input
			mUpsampledOutput1 = fn(mUpsampledInput1);
			mUpsampledOutput2 = fn(mUpsampledInput2);
			
			// downsample each processed row to 1x output 
			DSPVectorArray<OUT_ROWS> vy;
			for(int j=0; j < OUT_ROWS; ++j)
			{
				vy.row(j) = mDowners[j].downsample(mUpsampledOutput1.constRow(j), mUpsampledOutput2.constRow(j));
			}
			return vy;
		}
		
	private:
		std::array<HalfBandFilter, IN_ROWS> mUppers;
		std::array<HalfBandFilter, OUT_ROWS> mDowners;
		DSPVectorArray<IN_ROWS> mUpsampledInput1, mUpsampledInput2;
		DSPVectorArray<OUT_ROWS> mUpsampledOutput1, mUpsampledOutput2;
	};
	
	
	// Downsample2x is a function object that given a process function f, 
	// downsamples the input x by 2, applies f, upsamples and returns the result.
	// Since two DSPVectors of input are needed to create a single vector of downsampled input
	// to the wrapped function, this function has an entire DSPVector of delay in addition to 
	// the group delay of the allpass interpolation (about 6 samples).

	template<int IN_ROWS, int OUT_ROWS>
	class Downsample2x
	{
		typedef std::function<DSPVectorArray<OUT_ROWS>(const DSPVectorArray<IN_ROWS>)> ProcessFn;
		
	public:
		// operator() takes two arguments: a process function and an input DSPVectorArray. 
		// The optional argument DSPVectorArray<0>() allows passing only one argument in the case of a generator with 0 input rows.
		inline DSPVectorArray<OUT_ROWS> operator()(ProcessFn fn, const DSPVectorArray<IN_ROWS> vx = DSPVectorArray<0>())
		{
			DSPVectorArray<OUT_ROWS> vy;
			if(mPhase)
			{				
				// downsample each row of input to 1/2x buffers			
				for(int j=0; j < IN_ROWS; ++j)
				{
					mDownsampledInput.row(j) = mDowners[j].downsample(mInputBuffer.constRow(j), vx.constRow(j));
				}
				
				// process downsampled input
				mDownsampledOutput = fn(mDownsampledInput);
				
				// upsample each processed row to output 
				for(int j=0; j < OUT_ROWS; ++j)
				{
					// first half is returned
					vy.row(j) = mUppers[j].upsampleFirstHalf(mDownsampledOutput.constRow(j));		
					
					// second half is buffered
					mOutputBuffer.row(j) = mUppers[j].upsampleSecondHalf(mDownsampledOutput.constRow(j));	
				}
			}
			else
			{
				// store input
				mInputBuffer = vx;
				// return buffer
				vy = mOutputBuffer;
			}
			mPhase = !mPhase;
			return vy;
		}
		
	private:
		std::array<HalfBandFilter, IN_ROWS> mDowners;
		std::array<HalfBandFilter, OUT_ROWS> mUppers;
		DSPVectorArray<IN_ROWS> mInputBuffer;
		DSPVectorArray<OUT_ROWS> mOutputBuffer;
		DSPVectorArray<IN_ROWS> mDownsampledInput;
		DSPVectorArray<OUT_ROWS> mDownsampledOutput;
		bool mPhase{false};
	};
	
	
	// ----------------------------------------------------------------
	// OverlapAdd TODO
	
	template<int LENGTH, int DIVISIONS, int IN_ROWS, int OUT_ROWS>
	class OverlapAdd
	{
		typedef std::function<DSPVectorArray<OUT_ROWS>(const DSPVectorArray<IN_ROWS>)> ProcessFn;

	public:
		inline DSPVectorArray<OUT_ROWS> operator()(ProcessFn fn, const DSPVectorArray<IN_ROWS> vx)
		{
		}


	private:
		//MLSignal mHistory;
		const DSPVector& mWindow;
	};
	
	
} // namespace ml

