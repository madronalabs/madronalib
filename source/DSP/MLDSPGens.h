//
//  MLDSPGens.h
//  madronalib
//
//  Created by Randy Jones on 4/14/2016
//
//

#pragma once

#include "MLDSPOps.h"

namespace ml
{		
	// MLTEST
	class TickSource
	{
	public:
		TickSource(int p) : mCounter(0), mPeriod(p) {}
		~TickSource() {}
		
		inline DSPVector operator()()
		{
			DSPVector vy;
			for(int i=0; i<kFloatsPerDSPVector; ++i)
			{
				float fy = 0;
				if(++mCounter >= mPeriod)
				{
					mCounter = 0; 
					fy = 1;
				}
				vy[i] = fy;
			}
			return vy;
		}
		int mCounter;
		int mPeriod;
	};
	
	
	class RandomSource
	{
	public:
		RandomSource() : mSeed(0) {}
		~RandomSource() {}
		
		inline void step()
		{
			mSeed = mSeed * 0x0019660D + 0x3C6EF35F;
		}

		inline float getSample()
		{
			step();
			uint32_t temp = (mSeed >> 9) & 0x007FFFFF;
			temp &= 0x007FFFFF;
			temp |= 0x3F800000;
			float* pf = reinterpret_cast<float*>(&temp);
			return (*pf)*2.f - 3.f;			
		}
		
		inline uint32_t getIntSample()
		{
			step();
			return mSeed;
		}
		
		inline DSPVector operator()()
		{
			DSPVector y;
			for(int i=0; i<kFloatsPerDSPVector; ++i)
			{
				y[i] = getSample();
			}
			return y;
		}
		
		void reset() { mSeed = 0; }
		
	private:
		uint32_t mSeed = 0;
		
	};
	
	/*
	 Vector Ops
	 =======	 
	 
	 
	 Vector Gens
	 utils (functors)
	 these can use CRTP / operator(). 
	 -----------
	 
	 0 operands (generators):
	 RandomSource
	 sineOsc
	 TriOsc
	 PhaseOsc
	 
	 1 operand (filters)
	 differentiator
	 integrator
	 FixedDelay 
	 LinearDelay
	 AllpassDelay (or, interp. set by function? allpass interp. has state. )	 
	 FDN	 
	 Downsampler2
	 upsampler2
	 inline DSPVector SVF::operator();
	 biquad
	 onepole
	 asymmetriconepole
	 
	 ramp generator
	 quadratic generator
	 
	 
	 banks:
	 ----
	 sinebank
	 phasebank
	 SVFbank
	 biquadbank
	*/


}
