// MLDSPGens.h
// madronalib
//
// Created by Randy Jones on 4/14/2016
// 
// DSP generators: functor objects implementing an inline DSPVector operator()() ,
// in order to make time-varying signals. Generators all have some state, for example
// the frequency of an oscillator or the seed in a noise generator. Otherwise they 
// would be DSPOps. 
// 
// These objects are for building fixed DSP graphs in a functional style. The compiler should 
// have many opportunities to optimize these graphs. For dynamic graphs changeable at runtime,
// see MLProcs. In general MLProcs will be written using DSPGens, DSPOps, DSPFilters.

#pragma once

#include "MLDSPOps.h"

namespace ml
{		
	class TickSource
	{
	public:
		TickSource(int p) : mCounter(p), mPeriod(p) {}
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
		
		inline uint32_t getIntSample()
		{
			step();
			return mSeed;
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
		
		inline DSPVector operator()()
		{
			DSPVector y;
			for(int i=0; i<kFloatsPerDSPVector; ++i)
			{
				step();
				uint32_t temp = (mSeed >> 9) & 0x007FFFFF;
				temp &= 0x007FFFFF;
				temp |= 0x3F800000;
				float* pf = reinterpret_cast<float*>(&temp);
				y[i] = (*pf)*2.f - 3.f;		
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
	
	// if up at MLProc level, all outputs have fixed sizes, procs like sine16, sine64, sine256, sine1024 can be used
	// this is probably not the worst thing
	// what is penalty of dynamic sizing? 
	// 
	// proc can have a "size" switch on it that creates different gens internally. but, resizing graph dynamically is complex.
	// outputs auto-sum to smaller inputs?
	
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
	
	// ----------------------------------------------------------------
	// LinearGlide
	
	// convert a scalar float input into a DSPVector with linear slew.
	// to allow optimization, glide time is quantized to DSPVectors.
	// Note that a onepole or similar is not used because we must reach 
	// the actual value in a finite time.
	
	constexpr float unityRampFn(int i){ return (i + 1)/static_cast<float>(kFloatsPerDSPVector);  }
	constexpr DSPVector kUnityRampVec(unityRampFn);
	
	class LinearGlide
	{
	public:
		LinearGlide() : 
		mCurrVec(0.f),
		mStepVec(0.f),
		mSecondsPerGlide(0.01f), 
		mTargetValue(0.f),
		mVectorsRemaining(0)
		{
			calcParams();
		}
		
		void calcParams()
		{
			float samplesPerGlide = mSecondsPerGlide*mSr;
			mVectorsPerGlide = samplesPerGlide/kFloatsPerDSPVector;
			if(mVectorsPerGlide < 1) mVectorsPerGlide = 1;
			mDyPerVector = 1.0f/(mVectorsPerGlide + 0.f);
		}
		
		void setSampleRate(float sr) 
		{ 
			mSr = sr; 
			calcParams();
		}
		
		void setGlideTime(float t)
		{ 
			mSecondsPerGlide = t;
			calcParams();
		}
		
		void setInput(float f)
		{
			// because the last element of kUnityRampVec is 1, the last element
			// of mCurrVec at the end of a glide should be the target value.
			float currentValue = mCurrVec[kFloatsPerDSPVector - 1];
			if(f != currentValue)
			{
				mTargetValue = f;
				
				// start counter
				mVectorsRemaining = mVectorsPerGlide;	
			}
		}
		
		DSPVector operator()()
		{
			if(mVectorsRemaining <= 0)
			{
				return DSPVector(mTargetValue);	
			}
			else if(mVectorsRemaining == mVectorsPerGlide)
			{
				// start glide: get change in output value per vector
				float currentValue = mCurrVec[kFloatsPerDSPVector - 1];
				float dydv = (mTargetValue - currentValue)*mDyPerVector;
				
				// get constant step vector
				mStepVec = DSPVector(dydv);
				
				// setup current vector with first interpolation ramp. 
				mCurrVec = DSPVector(currentValue) + kUnityRampVec*DSPVector(dydv);
				
				mVectorsRemaining--;
			}
			else 
			{
				// continue glide
				// Note that repeated adding will create some error in target value. Because
				// we return the target value explicity when we are done, this won't be a problem in 
				// reasonably short glides.
				mCurrVec += mStepVec;
				mVectorsRemaining--;
			}
			
			return mCurrVec;
		}
		
		DSPVector mCurrVec;
		DSPVector mStepVec;
		
		float mSecondsPerGlide;
		float mTargetValue;
		float mSr;
		float mDyPerVector;
		int mVectorsPerGlide;	
		int mVectorsRemaining;
	};


}
