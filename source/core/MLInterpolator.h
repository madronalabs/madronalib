//
//  MLInterpolator.h
//  madronalib
//
//  Created by Randy Jones on 12/27/15.
//
//

#pragma once

// FIRST DRAFT! only 2D and linear. 

const int kInterpolationDegree = 1;

#include "MLDebug.h"
#include "MLSignal.h"
#include "MLProjection.h"

namespace ml
{
	class Interpolator
	{
	public:
		Interpolator() : mCurrentFrameIdx(0), mTargetFrameIdx(0), mTest(0)
		{}
		~Interpolator() {}
		
		void setTargetFrames(int f)
		{
			mCurrentFrameIdx = 0;
			mTargetFrameIdx = f;
		}
		
		MLSignal operator()(const MLSignal& target, int frames)
		{
			int frameSize = target.getHeight();
			MLSignal requiredDims{static_cast<float>(kInterpolationDegree + 1), static_cast<float>(frameSize)};
			
			if(mHistory.getDims() != requiredDims)
			{
				mHistory.setDims(requiredDims);
			}

			// mHistory = shiftLeft(mHistory);
			float* pStart = mHistory.getBuffer();
			std::copy(pStart + 1, pStart + mHistory.getSize() - 1, pStart);
			
			// write most recent history column
			// mHistory.column(kInterpolationDegree) = target;
			float* pLastCol = mHistory.getBuffer() + kInterpolationDegree;
			int strideBits = mHistory.getWidthBits();			
			for(int j=0; j<frameSize; ++j)
			{
				pLastCol[j << strideBits] = target(0, j);
			}

			MLSignal y(frames, frameSize);
			IntervalProjection frameToUnity({0, frames - 1}, {0, 1});
			
			// MLTEST linear mapping, ignore target frames and use current frames
			for(int j=0; j<frameSize; ++j)
			{
				for(int i=0; i<frames; ++i)
				{
					float mix = frameToUnity(i);
					y(i, j) = lerp(mHistory(kInterpolationDegree - 1, j), mHistory(kInterpolationDegree, j), mix);
				}
			}
			
			// TODO another way to use signal ctor as return statement:
			// MLSignal(float*, int w, int h, int stridex, int stridey, std::function<float(float)> const& f)
			/*
			mTest++;
			if(mTest > 1000)
			{
			y.dump(debug(), true);
				mTest = 0;
			}*/
			return y;
		}
		
	private:
		
		// history-- enough frames to satisfy degree of chosen interpolator fn are needed. 
		// vertical size will be the size of whatever target was last received.
		MLSignal mHistory;
		
		int mCurrentFrameIdx;
		int mTargetFrameIdx;
		int mTest;
	};
}
