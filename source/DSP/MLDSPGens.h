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
	// generate a single-sample tick every n samples.
	
	class TickGen
	{
	public:
		TickGen(int p) : mCounter(p), mPeriod(p) {}
		~TickGen() {}
		
		inline void setPeriod(int p)
		{
			mPeriod = p;
		}
		
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
	
	// generate an integer ramp repeating every n samples.

	class RampGen
	{
	public:
		RampGen(int p) : mCounter(p), mPeriod(p) {}
		~RampGen() {}
		
		inline void setPeriod(int p)
		{
			mPeriod = p;
		}
		
		inline DSPVectorInt operator()()
		{
			DSPVectorInt vy;
			for(int i=0; i<kFloatsPerDSPVector; ++i)
			{
				float fy = 0;
				if(++mCounter >= mPeriod)
				{
					mCounter = 0; 
					fy = 1;
				}
				vy[i] = mCounter;
			}
			return vy;
		}
		int mCounter;
		int mPeriod;
	};
	
	
	// antialiased ImpulseGen TODO 
	
	
	class NoiseGen
	{
	public:
		NoiseGen() : mSeed(0) {}
		~NoiseGen() {}
		
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
			uint32_t temp = ((mSeed >> 9) & 0x007FFFFF) | 0x3F800000;
			return (*reinterpret_cast<float*>(&temp))*2.f - 3.f;	
		}
		
		// TODO SIMD
		inline DSPVector operator()()
		{
			DSPVector y;
			for(int i=0; i<kFloatsPerDSPVector; ++i)
			{
				step();
				uint32_t temp = ((mSeed >> 9) & 0x007FFFFF) | 0x3F800000;
				y[i] = (*reinterpret_cast<float*>(&temp))*2.f - 3.f;		
			}
			return y;
		}
		
		void reset() { mSeed = 0; }
		
	private:
		uint32_t mSeed = 0;
	};
	
	// if up at MLProc level, all outputs have fixed sizes, procs like sine16, sine64, sine256, sine1024 can be used
	// this is probably not the worst thing
	// what is penalty of dynamic sizing? 
	// 
	// proc can have a "size" switch on it that creates different gens internally. but, resizing graph dynamically is complex.
	// outputs auto-sum to smaller inputs?
	
	/*
	 
	 0 operands (generators):
	 sineOsc
	 TriOsc
	 PhaseOsc
	 
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
	
	
	// super slow + accurate sine generator for testing
	
	class TestSineGen
	{		
		float mOmega{0};
		
	public:
		void clear()
		{
			mOmega = 0;
		}
		
		DSPVector operator()(const DSPVector freq)
		{	
			DSPVector vy;
			
			for(int i=0; i<kFloatsPerDSPVector; ++i)
			{
				float step = ml::kTwoPi*freq[i];
				mOmega += step;
				if(mOmega > ml::kTwoPi) mOmega -= ml::kTwoPi;
				vy[i] = sinf(mOmega);
			}
			return vy;
		}
	};
  
	
	// SineGen approximates a sine using Taylor series. There is distortion in odd harmonics
	// only, with the 3rd harmonic at about -40dB.
	
	class SineGen
	{		
		static constexpr int32_t kZeroPhase = -(2<<29);
		
		int32_t mOmega32{kZeroPhase};
		
	public:
		void clear()
		{
			mOmega32 = kZeroPhase;
		}
		
		// this sine generator makes a looping counter by letting a 32 bit word overflow.
		DSPVector operator()(const DSPVector freq)
		{	
			constexpr float sqrt2(const_math::sqrt(2.0f));
			constexpr float range(sqrt2 - sqrt2*sqrt2*sqrt2/6.f);
			constexpr float domain(sqrt2*4.f);
			constexpr float intDomain(const_math::pow(2., 32.));
			
			// TODO verify compile time creation 
			DSPVector domainScaleV(domain/intDomain);
			DSPVector domainOffsetV(sqrt2);
			DSPVector flipOffsetV(sqrt2*2.f);
			DSPVector oneSixthV(1.0f/6.f);
			DSPVector scaleV(1.0f/range);
			DSPVector zeroV(0.f);
			DSPVector oneV(1.f);
			
			DSPVector srDomainFreq = freq*DSPVector(intDomain);
			DSPVectorInt step32V = roundFloatToInt(srDomainFreq); 
			DSPVectorInt omega32V;
			
			// accumulate 32-bit phase with wrap
			for (int n = 0; n < kIntsPerDSPVector; ++n)
			{
				mOmega32 += step32V[n];
				omega32V[n] = mOmega32;
			}
			
			DSPVector phaseV = intToFloat(omega32V);	
			DSPVector omegaV = phaseV*(domainScaleV) + (domainOffsetV);
			
			// reverse upper half of phasor to get triangle
			// equivalent to: if (mOmega32 > 0) x = flipOffset - fOmega; else x = fOmega;
			DSPVectorInt maskV = greaterThan(phaseV, zeroV);
			omegaV = select((flipOffsetV) - omegaV, omegaV, maskV); 
			
			// convert triangle to sine approx. 
			return scaleV*omegaV*(oneV - omegaV*omegaV*oneSixthV);
		}
	};
	
	
	// ----------------------------------------------------------------
	// LinearGlide
	
	// convert a scalar float input into a DSPVector with linear slew.
	// to allow optimization, glide time is quantized to DSPVectors.
	// Note that a onepole or other IIR filter is not used because we must reach
	// the actual value in a finite time.

	constexpr float unityRampFn(int i){ return (i + 1)/static_cast<float>(kFloatsPerDSPVector);  }
	ConstDSPVector kUnityRampVec{unityRampFn};
	
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
		
		// TODO rename
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

	
	// GenBanks can go here
	
	/*
	 banks:
	----
	sinebank
noisebank
	 oscbank

	 
	 
	 template<int VECTORS>
	 class SineBank
	 {
	 // float will be promoted to Matrix of size 1 for single argument
	 SineBank<VECTORS>(Matrix f) { setFrequency(f); clear(); }
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
	
	// 
}
