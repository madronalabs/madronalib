//
// MLDSPFilters.h
// madronalib
//
// Created by Randy Jones on 4/14/2016
//
// DSP filters: functor objects implementing an operator()(DSPVector input).
// All these filters have some state, otherwise they would be DSPOps.
// 
// These objects are for building fixed DSP graphs in a functional style. The compiler should 
// have many opportunities to optimize these graphs. For dynamic graphs changeable at runtime,
// see MLProcs. In general MLProcs will be written using DSPGens, DSPOps, DSPFilters.
//
// Filter cutoffs are set by a parameter omega, equal to frequency / sample rate. This lets
// filter objects be unaware of the sample rate, resulting in less code overall.
// For all filters, k is a damping parameter equal to 1/Q where Q is the analog filter "quality."
// For bell and shelf filters, gain is specified as an output / input ratio A. 

#pragma once

#include "MLDSPOps.h"
#include "MLDSPGens.h"

#include <vector>

namespace ml
{
	// use this, not dBToAmp for calculating filter gain parameter A.
	inline float dBToGain(float dB)
	{
		return powf(10.f, dB/40.f);
	}
	
	// --------------------------------------------------------------------------------
	// utility filters implemented as SVF variations
	// Thanks to Andrew Simper [www.cytomic.com] for sharing his work over the years.

	class Lopass
	{
		struct _coeffs
		{
			float g0, g1, g2;
		};
		
		float ic1eq{0};
		float ic2eq{0};
		
	public:						
		_coeffs mCoeffs{0};
		
		static _coeffs coeffs (float omega, float k)
		{
			float piOmega = kPi*omega;
			float s1 = sinf(piOmega);
			float s2 = sinf(2.0f*piOmega);
			float nrm = 1.0f/(2.f + k*s2);
			float g0 = s2*nrm;
			float g1 = (-2.f*s1*s1 - k*s2)*nrm;
			float g2 = (2.0f*s1*s1)*nrm;			
			return {g0, g1, g2};
		}

		inline DSPVector operator()(const DSPVector vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float t0 = v0 - ic2eq;
				float t1 = mCoeffs.g0*t0 + mCoeffs.g1*ic1eq;
				float t2 = mCoeffs.g2*t0 + mCoeffs.g0*ic1eq;
				float v2 = t2 + ic2eq;
				ic1eq += 2.0f*t1;
				ic2eq += 2.0f*t2;
				vy[n] = v2;
			}
			return vy;
		}
	};
	
	class Hipass
	{
		struct _coeffs
		{
			float g0, g1, g2, k;
		};
		
		float ic1eq{0};
		float ic2eq{0};
		
	public:						
		_coeffs mCoeffs{0};

		static _coeffs coeffs (float omega, float k)
		{
			float piOmega = kPi*omega;
			float s1 = sinf(piOmega);
			float s2 = sinf(2.0f*piOmega);
			float nrm = 1.0f/(2.f + k*s2);
			float g0 = s2*nrm;
			float g1 = (-2.f*s1*s1 - k*s2)*nrm;
			float g2 = (2.0f*s1*s1)*nrm;			
			return {g0, g1, g2, k};
		}
		
		inline DSPVector operator()(const DSPVector vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float t0 = v0 - ic2eq;
				float t1 = mCoeffs.g0*t0 + mCoeffs.g1*ic1eq;
				float t2 = mCoeffs.g2*t0 + mCoeffs.g0*ic1eq;
				float v1 = t1 + ic1eq;
				float v2 = t2 + ic2eq;
				ic1eq += 2.0f*t1;
				ic2eq += 2.0f*t2;
				vy[n] = v0 - mCoeffs.k*v1 - v2; 
			}		
			return vy;
		}
	};
	
	class Bandpass
	{
		struct _coeffs
		{
			float g0, g1, g2;
		};
				
		float ic1eq{0};
		float ic2eq{0};
		
	public:						
		_coeffs mCoeffs{0};

		static _coeffs coeffs (float omega, float k)
		{
			float piOmega = kPi*omega;
			float s1 = sinf(piOmega);
			float s2 = sinf(2.0f*piOmega);
			float nrm = 1.0f/(2.f + k*s2);
			float g0 = s2*nrm;
			float g1 = (-2.f*s1*s1 - k*s2)*nrm;
			float g2 = (2.0f*s1*s1)*nrm;			
			return {g0, g1, g2};
		}
		
		inline DSPVector operator()(const DSPVector vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float t0 = v0 - ic2eq;
				float t1 = mCoeffs.g0*t0 + mCoeffs.g1*ic1eq;
				float t2 = mCoeffs.g2*t0 + mCoeffs.g0*ic1eq;
				float v1 = t1 + ic1eq;
				ic1eq += 2.0f*t1;
				ic2eq += 2.0f*t2;
				vy[n] = v1; 
			}		
			return vy;
		}
	};
	
	class LoShelf
	{
		struct _coeffs
		{
			float a1, a2, a3, m1, m2;
		};
		
		float ic1eq{0};
		float ic2eq{0};		
		
	public:						
		_coeffs mCoeffs{0};

		static _coeffs coeffs (float omega, float k, float A)
		{
			float piOmega = kPi*omega;
			float g = tanf(piOmega)/sqrtf(A);
			float a1 = 1.f/(1.f + g*(g + k));
			float a2 = g*a1;
			float a3 = g*a2;
			float m1 = k*(A - 1.f);
			float m2 = (A*A - 1.f);
			return {a1, a2, a3, m1, m2};
		}
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float v3 = v0 - ic2eq;
				float v1 = mCoeffs.a1*ic1eq + mCoeffs.a2*v3;
				float v2 = ic2eq + mCoeffs.a2*ic1eq + mCoeffs.a3*v3;
				ic1eq = 2*v1 - ic1eq;
				ic2eq = 2*v2 - ic2eq;				
				vy[n] = v0 + mCoeffs.m1*v1 + mCoeffs.m2*v2;
			}		
			return vy;
		}
	};
	
	class HiShelf
	{
		struct _coeffs
		{
			float a1, a2, a3, m0, m1, m2;
		};
		
		float ic1eq{0};
		float ic2eq{0};		
		
	public:			
		_coeffs mCoeffs{0};
		
		static _coeffs coeffs (float omega, float k, float A)
		{
			float piOmega = kPi*omega;
			float g = tanf(piOmega)*sqrtf(A);
			float a1 = 1.f/(1.f + g*(g + k));
			float a2 = g*a1;
			float a3 = g*a2;
			float m0 = A*A;
			float m1 = k*(1.f - A)*A;
			float m2 = (1.f - A*A);
			return {a1, a2, a3, m0, m1, m2};
		}
		
		inline DSPVector operator()(const DSPVector vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float v3 = v0 - ic2eq;				
				float v1 = mCoeffs.a1*ic1eq + mCoeffs.a2*v3;
				float v2 = ic2eq + mCoeffs.a2*ic1eq + mCoeffs.a3*v3;				
				ic1eq = 2*v1 - ic1eq;
				ic2eq = 2*v2 - ic2eq;								
				vy[n] = mCoeffs.m0*v0 + mCoeffs.m1*v1 + mCoeffs.m2*v2;
			}		
			return vy;
		}
	};
	
	class Bell
	{
		struct _coeffs
		{
			float a1, a2, a3, m1;
		};
		
		float ic1eq{0};
		float ic2eq{0};		
		
	public:			
		_coeffs mCoeffs{0};
		
		static _coeffs coeffs (float omega, float k, float A)
		{
			float kc = k/A; // correct k 
			float piOmega = kPi*omega;
			float g = tanf(piOmega);
			float a1 = 1.f/(1.f + g*(g + kc));
			float a2 = g*a1;
			float a3 = g*a2;
			float m1 = kc*(A*A - 1.f);
			return {a1, a2, a3, m1};
		}
		
		inline DSPVector operator()(const DSPVector vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float v3 = v0 - ic2eq;				
				float v1 = mCoeffs.a1*ic1eq + mCoeffs.a2*v3;
				float v2 = ic2eq + mCoeffs.a2*ic1eq + mCoeffs.a3*v3;				
				ic1eq = 2*v1 - ic1eq;
				ic2eq = 2*v2 - ic2eq;								
				vy[n] = v0 + mCoeffs.m1*v1;
			}		
			return vy;
		}
	};

	// A one pole filter. see https://ccrma.stanford.edu/~jos/fp/One_Pole.html
	
	class OnePole
	{
		struct _coeffs
		{
			float a0, b1;
		};
		
		float y1{0};

	public:
		_coeffs mCoeffs{0};
		
		static _coeffs coeffs (float omega)
		{
			float x = expf(-omega*kTwoPi);
			return {1.f - x, x};
		}
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				y1 = mCoeffs.a0*vx[n] + mCoeffs.b1*y1;
				vy[n] = y1;
			}
			return vy;
		}
	};

	// A one-pole, one-zero filter to attenuate DC. 
	// Works well, but beware of its effects on bass sounds. An omega of 0.05 is a good starting point.
	// see https://ccrma.stanford.edu/~jos/fp/DC_Blocker.html for more. 

	class DCBlocker
	{
		typedef float _coeffs;
		float x1{0};
		float y1{0};
		
	public:
		_coeffs mCoeffs{0};

		static _coeffs coeffs (float omega)
		{
			return cosf(omega);
		}
		
		inline DSPVector operator()(const DSPVector vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				const float x0 = vx[n];
				const float y0 = x0 - x1 + mCoeffs*y1;
				y1 = y0;
				x1 = x0;
				vy[n] = y0;
			}
			return vy;
		}
	};

	
	// Differentiator 
	
	class Differentiator
	{
		float x1{0};
		
	public:
		inline DSPVector operator()(const DSPVector vx)
		{
			DSPVector vy;
			vy[0] = x1 - vx[0];
			for(int n=1; n<kFloatsPerDSPVector; ++n)
			{
				vy[n] = vx[n - 1] - vx[n];
			}
			x1 = vx[kFloatsPerDSPVector];
			return vy;
		}
	};

	
	// Integrator 
	
	class Integrator
	{
		float y1{0};
		
	public:
		// set leak to a value such as 0.001 for stability
		float mLeak{0};
		
		inline DSPVector operator()(const DSPVector vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				y1 -= y1*mLeak;
				y1 += vx[n];
				vy[n] = y1;
			}
			return vy;
		}
	};
	
	
	// IntegerDelay delays a signal a whole number of samples.
	
	static constexpr int kDefaultDelaySize = 1024;
	
	class IntegerDelay
	{
		std::vector<float> mBuffer;
		int mIntDelayInSamples;
		uintptr_t mWriteIndex;
		uintptr_t mLengthMask;
		
	public:
		IntegerDelay() { setDelayInSamples(kDefaultDelaySize); }
		IntegerDelay(int d) { setDelayInSamples(d); }
		~IntegerDelay() {}
		
		inline void setDelayInSamples(int d) 
		{ 
			int dd = std::max(d, 0); 	
			if(dd > mBuffer.size())
			{
				setMaxDelayInSamples(dd);
			}
			mIntDelayInSamples = dd;
		}
		
		void setMaxDelayInSamples(int dMax)
		{
			int newSize = 1 << bitsToContain(dMax + kFloatsPerDSPVector);
			mBuffer.resize(newSize);
			mLengthMask = newSize - 1;
			mWriteIndex = 0;
			clear();
		}
		
		inline void clear()
		{
			std::fill(mBuffer.begin(), mBuffer.end(), 0.f);
		}
		
		inline DSPVector operator()(const DSPVector vx)
		{
			// write
			uintptr_t writeEnd = mWriteIndex + kFloatsPerDSPVector;
			if(writeEnd <= mLengthMask + 1)
			{
				const float* srcStart = vx.getConstBuffer();
				std::copy(srcStart, srcStart + kFloatsPerDSPVector, mBuffer.data() + mWriteIndex);
			}
			else
			{
				uintptr_t excess = writeEnd - mLengthMask - 1; 
				const float* srcStart = vx.getConstBuffer();
				const float* srcSplice = srcStart + kFloatsPerDSPVector - excess;
				const float* srcEnd = srcStart + kFloatsPerDSPVector;
				std::copy(srcStart, srcSplice, mBuffer.data() + mWriteIndex);
				std::copy(srcSplice, srcEnd, mBuffer.data());
			}
			
			// read			
			DSPVector vy; 
			uintptr_t readStart = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
			uintptr_t readEnd = readStart + kFloatsPerDSPVector;
			float* srcBuf = mBuffer.data();
			if(readEnd <= mLengthMask + 1)
			{
				std::copy(srcBuf + readStart, srcBuf + readEnd, vy.getBuffer());
			}
			else
			{
				uintptr_t excess = readEnd - mLengthMask - 1; 
				uintptr_t readSplice = readStart + kFloatsPerDSPVector - excess;
				float* pDest = vy.getBuffer();
				std::copy(srcBuf + readStart, srcBuf + readSplice, pDest);
				std::copy(srcBuf, srcBuf + excess, pDest + (kFloatsPerDSPVector - excess));
			}
			
			// update index
			mWriteIndex += kFloatsPerDSPVector;
			mWriteIndex &= mLengthMask;
			return vy;
		}
		
		inline DSPVector operator()(const DSPVector x, const DSPVector delay)
		{
			DSPVector y;
			
			for (int n=0; n<kFloatsPerDSPVector; ++n)
			{
				// write
				mBuffer[mWriteIndex] = x[n];
				
				// read
				mIntDelayInSamples = static_cast<int>(delay[n]);
				uintptr_t readIndex = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
				
				y[n] = mBuffer[readIndex];
				mWriteIndex++;
				mWriteIndex &= mLengthMask;
			}
						
			return y;			
		}
		
		inline float processSample(float x)
		{
			// write
			mBuffer[mWriteIndex] = x;
			
			// read			
			uintptr_t readIndex = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
			float y = mBuffer[readIndex];
			
			// update index
			mWriteIndex++;
			mWriteIndex &= mLengthMask;
			return y;
		}
	};
	

	// First order allpass section with a single sample of delay.
	
	class Allpass1
	{		
	private:
		float x1{0}, y1{0};
		
	public:
		float mCoeffs;
		
		Allpass1() : mCoeffs(0.f){}
		Allpass1(float a) : mCoeffs(a){}
		~Allpass1(){}
		
		// get allpass coefficient from a delay fraction d.
		// to minimize modulation noise, d should be in the range [0.618 - 1.618].
		static float coeffs (float d)
		{
			// return 2nd order approx around 1 to (1.f - d) / (1.f + d)
			float xm1 = (d - 1.f);
			return -0.53f*xm1 + 0.24f*xm1*xm1; 
		}
		
		inline float processSample(const float x)
		{
			// one-multiply form. see https://ccrma.stanford.edu/~jos/pasp/One_Multiply_Scattering_Junctions.html
			float y = x1 + (x - y1)*mCoeffs;            
			x1=x;
			y1=y;
			return y;
		}
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{          
				vy[n] = processSample(vx[n]);
			}
			return vy;
		}
	};
	
	
	// Combining the integer delay and first order allpass section 
	// gives us an allpass-interpolated fractional delay. In general, modulating the delay time 
	// will change the allpass coefficient, producing clicks in the output.
	
	class FractionalDelay
	{
		IntegerDelay mIntegerDelay;
		Allpass1 mAllpassSection;
		float mDelayInSamples;
		
	public:
		FractionalDelay() { setDelayInSamples(kDefaultDelaySize); }
		FractionalDelay(float d) { setDelayInSamples(d); }
		~FractionalDelay() {}
				
		inline void clear()
		{
			mIntegerDelay.clear();
		}
		
		inline void setDelayInSamples(float d) 
		{ 			
			mDelayInSamples = d;
			float fDelayInt = floorf(d);
			int delayInt = fDelayInt;
			float delayFrac = d - fDelayInt;
			
			// constrain D to [0.618 - 1.618];
			if (delayFrac < 0.618f)
			{
				delayFrac += 1.f;
				delayInt -= 1;
			}
			mIntegerDelay.setDelayInSamples(delayInt);
			mAllpassSection.mCoeffs = Allpass1::coeffs(delayFrac);
		}
		
		// return the input signal, delayed by the constant delay time mDelayInSamples.
		inline DSPVector operator()(const DSPVector vx)
		{
			return mAllpassSection(mIntegerDelay(vx));
		}
		
		// return the input signal, delayed by the varying delay time vDelayInSamples.
		inline DSPVector operator()(const DSPVector vx, const DSPVector vDelayInSamples)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				setDelayInSamples(vDelayInSamples[n]);
				vy[n] = mAllpassSection.processSample(mIntegerDelay.processSample(vx[n]));
			}
			return vy;
		}
		
		// return the input signal, delayed by the varying delay time vDelayInSamples, but only allow changes
		// to the delay time when vChangeTicks is nonzero.
		inline DSPVector operator()(const DSPVector vx, const DSPVector vDelayInSamples, const DSPVectorInt vChangeTicks)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				if(vChangeTicks[n] != 0)
				{		
					setDelayInSamples(vDelayInSamples[n]);
				}
				
				vy[n] = mAllpassSection.processSample(mIntegerDelay.processSample(vx[n]));
			}
			return vy;
		}
	};
	
	
	// Crossfading two allpass-interpolated delays allows modulating the delay 
	// time without clicks. See "A Lossless, Click-free, Pitchbend-able Delay Line Loop Interpolation Scheme", 
	// Van Duyne, Jaffe, Scandalis, Stilson, ICMC 1997.
	
	static constexpr int kPBDFadePeriod = 32; 
	static constexpr std::array<int, kPBDFadePeriod> kPBDFadeTable{ {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1} };
	static constexpr float kPBDFadeMax = 16.f;

	class PitchbendableDelay
	{
		FractionalDelay mDelay1, mDelay2;
		ml::RampGen rampFn{kPBDFadePeriod};		
		
	public:
		PitchbendableDelay() : mDelay1(kDefaultDelaySize), mDelay2(kDefaultDelaySize)
		{
			mDelay1.setDelayInSamples(0);
			mDelay2.setDelayInSamples(0);
		}
		
		PitchbendableDelay(float d) : mDelay1(d), mDelay2(d)
		{
			mDelay1.setDelayInSamples(0);
			mDelay2.setDelayInSamples(0);
		}
		
		~PitchbendableDelay() {}
		
		inline DSPVector operator()(const DSPVector vInput, const DSPVector vDelayInSamples)
		{
			// get fade function
			DSPVectorInt vIntRamp = rampFn();
			DSPVector vFade = map([&](int f){ return kPBDFadeTable[f]/kPBDFadeMax; }, vIntRamp); // triangular
			
			// generate vectors of ticks indicating when delays can change
			// equality operators on vectors return 0 or 0xFFFFFFFF 
			// note: mDelay1's delay time will be 0 when the object is created and before the first half fade period.
			// so there is a warmup time of one half fade period: any input before this will be attenuated.
			DSPVectorInt vDelay1Changes = equal(intToFloat(vIntRamp), DSPVector(kPBDFadePeriod/2.f));
			DSPVectorInt vDelay2Changes = equal(intToFloat(vIntRamp), DSPVector(0.f));
			
			// run the fractional delays and crossfade the results.
			return lerp(mDelay1(vInput, vDelayInSamples, vDelay1Changes), mDelay2(vInput, vDelayInSamples, vDelay2Changes), vFade);
		}
	};
	
	
	// General purpose allpass filter with arbitrary delay length.
	// For efficiency, the minimum delay time is one DSPVector.
	
	template<typename DELAY_TYPE>
	class Allpass
	{
		DELAY_TYPE mDelay{kDefaultDelaySize};
		DSPVector vy1;
		
	public:
		float mGain{0.f};
		
		// use setDelayInSamples to set a constant delay time with DELAY_TYPE of IntegerDelay or FractionalDelay.
		inline void setDelayInSamples(float d) 
		{ 			
			mDelay.setDelayInSamples(d - kFloatsPerDSPVector);
		}
		
		// use with constant delay time.
		inline DSPVector operator()(const DSPVector vInput)
		{	
			DSPVector vGain(-mGain);
			DSPVector vDelayInput = vInput - vy1*vGain;
			DSPVector y = vDelayInput*vGain + vy1;
			vy1 = mDelay(vDelayInput);
			return y;
		}
		
		// use vDelayInSamples parameter to set a varying delay time with DELAY_TYPE = PitchbendableDelay.
		inline DSPVector operator()(const DSPVector vInput, const DSPVector vDelayInSamples)
		{	
			DSPVector vGain(-mGain);
			DSPVector vDelayInput = vInput - vy1*vGain;
			DSPVector y = vDelayInput*vGain + vy1;
			vy1 = mDelay(vDelayInput, vDelayInSamples - DSPVector(kFloatsPerDSPVector));
			return y;
		}
	};


	// FDN	
	// A general Feedback Delay Network with N delay lines connected in an NxN matrix.
	
	template<int SIZE>
	class FDN
	{
		std::array<IntegerDelay, SIZE> mDelays;
		std::array<OnePole, SIZE> mFilters; 
		std::array<DSPVector, SIZE> mDelayInputVectors{ { {DSPVector(0.f)} } }; 

	public:

		// feedback gains array is public—just copy values to set. 
		std::array<float, SIZE> mFeedbackGains{{0}};
		
		void setDelaysInSamples(std::array<float, SIZE> times)
		{			
			for(int n=0; n<SIZE; ++n)
			{
				// we have one DSPVector feedback latency, so compensate delay times for that.
				int len = times[n] - kFloatsPerDSPVector;
				len = max(1, len);
				mDelays[n].setDelayInSamples(len);
			}
		}
		
		void setFilterCutoffs(std::array<float, SIZE> omegas)
		{
			for(int n=0; n<SIZE; ++n)
			{
				mFilters[n].mCoeffs = ml::OnePole::coeffs(omegas[n]);
			}
		}
		
		// stereo output function
		DSPVectorArray<2> operator()(const DSPVector x)
		{
			// run delays, getting DSPVector for each delay 
			for(int n=0; n<SIZE; ++n)
			{
				mDelayInputVectors[n] = mDelays[n](mDelayInputVectors[n]);
			}
			
			// get output sum
			DSPVector sumR, sumL;
			for(int n=0; n<(SIZE&(~1)); ++n)
			{
				if(n&1)
				{
					sumL += mDelayInputVectors[n];
				}
				else
				{
					sumR += mDelayInputVectors[n];
				}
			}
			
			// inputs = input gains*input sample + filters(M*delay outputs)
			// The feedback matrix M is a unit-gain Householder matrix, which is just 
			// the identity matrix minus a constant k, where k = 2/size. Since multiplying this can be
			// simplified so much, you just see a few operations here, not a general 
			// matrix multiply.
			
			DSPVector sumOfDelays;	
			for(int n=0; n<SIZE; ++n)
			{
				sumOfDelays += mDelayInputVectors[n];
			}
			sumOfDelays *= DSPVector(2.0f/SIZE);
			
			for(int n=0; n<SIZE; ++n)
			{
				mDelayInputVectors[n] -= (sumOfDelays);
				mDelayInputVectors[n] = mFilters[n](mDelayInputVectors[n]) * DSPVector(mFeedbackGains[n]);
				mDelayInputVectors[n] += x;
			}	
			
			return append(sumL, sumR);
		}
	};
		
	
	// ----------------------------------------------------------------
	// Half Band Filter
	// Polyphase allpass filter used to upsample or downsample a signal by 2x.
	// Structure due to fred harris, A. G. Constantinides and Valenzuela.
	
	class HalfBandFilter
	{
	public:
		
		inline DSPVector upsampleFirstHalf(const DSPVector vx)
		{
			DSPVector vy;
			int i2 = 0;
			for(int i = 0; i < kFloatsPerDSPVector/2; ++i)
			{
				vy[i2++] = apa1.processSample(apa0.processSample(vx[i]));				
				vy[i2++] = apb1.processSample(apb0.processSample(vx[i]));
			}		
			return vy;
		}
		
		inline DSPVector upsampleSecondHalf(const DSPVector vx)
		{
			DSPVector vy;
			int i2 = 0;
			for(int i = kFloatsPerDSPVector/2; i < kFloatsPerDSPVector; ++i)
			{
				vy[i2++] = apa1.processSample(apa0.processSample(vx[i]));				
				vy[i2++] = apb1.processSample(apb0.processSample(vx[i]));
			}		
			return vy;
		}
		
		inline DSPVector downsample(const DSPVector vx1, const DSPVector vx2)
		{
			DSPVector vy;
			int i2 = 0;
			for(int i = 0; i < kFloatsPerDSPVector/2; ++i)
			{
				float a0 = apa1.processSample(apa0.processSample(vx1[i2]));
				float b0 = apb1.processSample(apb0.processSample(vx1[i2 + 1]));
				vy[i] = (a0 + b1)*0.5f;
				b1 = b0;
				i2 += 2;
			}	
			i2 = 0;			
			for(int i = kFloatsPerDSPVector/2; i < kFloatsPerDSPVector; ++i)
			{
				float a0 = apa1.processSample(apa0.processSample(vx2[i2]));
				float b0 = apb1.processSample(apb0.processSample(vx2[i2 + 1]));
				vy[i] = (a0 + b1)*0.5f;
				b1 = b0;
				i2 += 2;
			}	
			return vy;
		}
		
	private:
		
		// order=4, rejection=70dB, transition band=0.1. 
		Allpass1 apa0{0.07986642623635751f}, apa1{0.5453536510711322f}, apb0{0.28382934487410993f}, apb1{0.8344118914807379f};	
		float b1{0};
	};
	
} // namespace ml

