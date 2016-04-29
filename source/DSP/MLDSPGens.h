// MLDSPGens.h
// madronalib
//
// Created by Randy Jones on 4/14/2016
//
// 

#pragma once

#include "MLDSPOps.h"
#include "MLSignal.h"

namespace ml
{		
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
	
	// horiz -> vert -> horiz
	
	// aligned int32 vector
	
	// tiny MLSignal
	
	/*
	template<int VECTORS>
	class SineBank
	{
		// float will be promoted to MLSignal of size 1 for single argument
		SineBank<VECTORS>(MLSignal f) { setFrequency(f); clear(); }
		~SineBank<VECTORS>() {}
		
		inline DSPVectorArray<VECTORS> operator()()
		{
			DSPVectorArray<VECTORS> y;
			for(int j=0; j<VECTORS; ++j)
			{

			}			
			return y;
		}
		
	private:
		int32_t mOmega32, mStep32;
		float mInvSrDomain;
	};
	
	typedef SineBank<1> Sine;
*/
	
	/*
	 Vector Ops
	 =======	 
	 
	 Vector Gens
	 utils (functors)
	 -----------
	 
	 0 operands (generators):
	 sineOsc
	 TriOsc
	 PhaseOsc
	 
	 1 operand (filters)
	 differentiator
	 integrator
	 FixedDelay 
	 
	 VariableDelayWithCrossfade
	 
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
	 delaybank
	 hooooold on...	 
	 
	 a bank of raised cos generators can be for a granulator or shepard tone generator
	 
	 
	*/


}
