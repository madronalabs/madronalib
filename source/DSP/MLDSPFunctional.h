// madronaLib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// Here are some function objects that take DSP functions as parameters to operator() and apply
// the function in a different context, such as upsampled, overlap-added or in the frequency domain.

// TODO in DSPOps improve pack / unpack of rows for sending multiple sources to functions

#pragma once

#include "MLDSPFilters.h"

#include <functional>

namespace ml
{
	// Upsample2xFunction is a function object that given a process function f, 
	// upsamples the input x by 2, applies f, downsamples and returns the result.
	// the total delay from the resampling filters used is about 3 samples.	

	// NOTE: all these templates were written with separate in and out rows
	// template<int IN_ROWS, int OUT_ROWS>
	// but a compiler bug is preventing it from working on Windows.
	// TODO revisit

	template<int IN_ROWS>
	class Upsample2xFunction
	{
		static constexpr int OUT_ROWS = 1; // see above

		using inputType = const DSPVectorArray< IN_ROWS >;
		using outputType = DSPVectorArray< 1 >; // OUT_ROWS
		using ProcessFn = std::function< outputType(inputType) >;

	public:		
		// operator() takes two arguments: a process function and an input DSPVectorArray. 
		inline outputType operator()(ProcessFn fn, inputType vx)
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
			outputType vy;
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
	

	// Downsample2xFunction is a function object that given a process function f, 
	// downsamples the input x by 2, applies f, upsamples and returns the result.
	// Since two DSPVectors of input are needed to create a single vector of downsampled input
	// to the wrapped function, this function has an entire DSPVector of delay in addition to 
	// the group delay of the allpass interpolation (about 6 samples).

	//template<int IN_ROWS, int OUT_ROWS>
	template<int IN_ROWS>
	class Downsample2xFunction
	{
		static constexpr int OUT_ROWS = 1; // see above

		using inputType = const DSPVectorArray< IN_ROWS >;
		using outputType = DSPVectorArray< 1 >; // OUT_ROWS
		using ProcessFn = std::function< outputType(inputType) >;

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
	

	// OverlapAddFunction TODO
	/*
	template<int LENGTH, int DIVISIONS, int IN_ROWS, int OUT_ROWS>
	class OverlapAddFunction
	{
		typedef std::function<DSPVectorArray<OUT_ROWS>(const DSPVectorArray<IN_ROWS>)> ProcessFn;
		
	public:
		inline DSPVectorArray<OUT_ROWS> operator()(ProcessFn fn, const DSPVectorArray<IN_ROWS> vx)
		{
		}
		
	private:
		//Matrix mHistory;
		const DSPVector& mWindow;
	};
	*/
	
	// FeedbackDelayFunction
	// Wraps a function in a pitchbendable delay with feedback per row. 
	// Since the feedback adds the output of the function to its input, the function must input and output
	// the same number of rows.
	
	//template<int ROWS>
	class FeedbackDelayFunction
	{
		static constexpr int ROWS = 1; // see above
		using inputType = const DSPVectorArray< ROWS >;
		using outputType = DSPVectorArray< 1 >; // ROWS
		using ProcessFn = std::function< outputType(inputType) >;

	public:
		float feedbackGain{1.f};
		
		inline DSPVectorArray<ROWS> operator()(const DSPVectorArray<ROWS> vx, ProcessFn fn, const DSPVector vDelayTime)
		{				
			DSPVectorArray<ROWS> vFnOutput;						
			vFnOutput = fn(vx + vy1*DSPVectorArray<ROWS>(feedbackGain));
			
			for(int j=0; j < ROWS; ++j)
			{
				vy1.row(j) = mDelays[j](vFnOutput.row(j), vDelayTime - DSPVector(kFloatsPerDSPVector));				
			}
			return vFnOutput;
		}
		
	private:
		std::array<PitchbendableDelay, ROWS> mDelays;
		DSPVectorArray<ROWS> vy1;
	};
	
	
	// FeedbackDelayFunctionWithTap
	// Wraps a function in a pitchbendable delay with feedback per row. The function outputs a tap that
	// can be different from the feedback signal sent to the input.
	// Since the feedback adds the output of the function to its input, the function must input and output
	// the same number of rows.
	
	//template<int ROWS>
	class FeedbackDelayFunctionWithTap
	{
		static constexpr int ROWS = 1; // see above
		using inputType = const DSPVectorArray< ROWS >;
		using tapType = DSPVectorArray< ROWS >&;
		using outputType = DSPVectorArray< 1 >; // ROWS
		using ProcessFn = std::function< outputType(inputType, tapType) >;

	public:
		float feedbackGain{1.f};
		
		inline DSPVectorArray<ROWS> operator()(const DSPVectorArray<ROWS> vx, ProcessFn fn, const DSPVector vDelayTime)
		{				
			DSPVectorArray<ROWS> vFeedback;						
			DSPVectorArray<ROWS> vOutputTap;						
			vFeedback = fn(vx + vy1*DSPVectorArray<ROWS>(feedbackGain), vOutputTap);
			
			for(int j=0; j < ROWS; ++j)
			{
				vy1.row(j) = mDelays[j](vFeedback.row(j), vDelayTime - DSPVector(kFloatsPerDSPVector));				
			}
			return vOutputTap;
		}
		
	private:
		std::array<PitchbendableDelay, ROWS> mDelays;
		DSPVectorArray<ROWS> vy1;
	};


	
} // namespace ml

