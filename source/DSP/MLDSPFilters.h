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
#include <vector>
namespace ml
{
	// use this, not dBToAmp for calculating filter gain parameter A.
	float dBToGain(float dB)
	{
		return powf(10.f, dB/40.f);
	}
	
	// --------------------------------------------------------------------------------
	// utility filters implemented as SVF variations
	// Thanks to Andrew Simper [www.cytomic.com] for sharing his work over the years.
	
	// TODO: time-varying coefficients, time-varying operators for each SVF type

	class Lopass
	{
	private:
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

		inline DSPVector operator()(const DSPVector& vx)
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
	private:
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
		
		inline DSPVector operator()(const DSPVector& vx)
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
	private:		
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
		
		inline DSPVector operator()(const DSPVector& vx)
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
	private:	
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
	private:
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
				vy[n] = mCoeffs.m0*v0 + mCoeffs.m1*v1 + mCoeffs.m2*v2;
			}		
			return vy;
		}
	};
	
	class Bell
	{
	private:
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
				vy[n] = v0 + mCoeffs.m1*v1;
			}		
			return vy;
		}
	};

	// A one pole filter. see https://ccrma.stanford.edu/~jos/fp/One_Pole.html
	
	class OnePole
	{
	private:
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
	private:
		typedef float _coeffs;
		float x1{0};
		float y1{0};
		
	public:
		_coeffs mCoeffs{0};

		static _coeffs coeffs (float omega)
		{
			return cosf(omega);
		}
		
		inline DSPVector operator()(const DSPVector& vx)
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

	
	// IntegerDelay delays a signal a whole number of samples.
	
	class IntegerDelay
	{
	public:
		IntegerDelay(int d) { setDelayInSamples(d); }
		IntegerDelay() {}
		~IntegerDelay() {}
		
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
		
		inline void setDelayInSamples(int d) 
		{ 
			if(d > mBuffer.size())
			{
				setMaxDelayInSamples(d);
			}
			mIntDelayInSamples = d; 		
		}
		
		inline DSPVector operator()(DSPVector& x)
		{
			// write
			uintptr_t writeEnd = mWriteIndex + kFloatsPerDSPVector;
			if(writeEnd <= mLengthMask + 1)
			{
				const float* srcStart = x.getConstBuffer();
				std::copy(srcStart, srcStart + kFloatsPerDSPVector, mBuffer.data() + mWriteIndex);
			}
			else
			{
				uintptr_t excess = writeEnd - mLengthMask - 1; 
				const float* srcStart = x.getConstBuffer();
				const float* srcSplice = srcStart + kFloatsPerDSPVector - excess;
				const float* srcEnd = srcStart + kFloatsPerDSPVector;
				std::copy(srcStart, srcSplice, mBuffer.data() + mWriteIndex);
				std::copy(srcSplice, srcEnd, mBuffer.data());
			}
			
			// read			
			DSPVector y; 
			uintptr_t readStart = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
			uintptr_t readEnd = readStart + kFloatsPerDSPVector;
			float* srcBuf = mBuffer.data();
			if(readEnd <= mLengthMask + 1)
			{
				std::copy(srcBuf + readStart, srcBuf + readEnd, y.getBuffer());
			}
			else
			{
				uintptr_t excess = readEnd - mLengthMask - 1; 
				uintptr_t readSplice = readStart + kFloatsPerDSPVector - excess;
				float* pDest = y.getBuffer();
				std::copy(srcBuf + readStart, srcBuf + readSplice, pDest);
				std::copy(srcBuf, srcBuf + excess, pDest + (kFloatsPerDSPVector - excess));
			}
			
			// update index
			mWriteIndex += kFloatsPerDSPVector;
			mWriteIndex &= mLengthMask;
			return y;
		}
		
	private:
		// TODO look at small size stack optimization here
		std::vector<float> mBuffer;
		int mIntDelayInSamples;
		uintptr_t mWriteIndex;
		uintptr_t mLengthMask;
	};
	

	// First order allpass section
	
	class AllpassSection
	{		
	private:
		float x0{0}, x1{0}, y0{0}, y1{0};
		
	public:
		float mCoeffs;
		
		AllpassSection(float a) : mCoeffs(a){}
		~AllpassSection(){}
		
		// get allpass coefficient from the delay fraction d.
		// to minimize modulation noise, d should be in the range [0.618 - 1.618].
		static float coeffs (float d)
		{
			// return (1.f - d) / (1.f + d); // exact
			float xm1 = (d - 1.f);
			return -0.53f*xm1 + 0.24f*xm1*xm1; // 2nd order approx around 1			
		}
		
		// needed?
		inline float processSample(const float x)
		{
			x1=x0;
			y1=y0;
			x0=x;
			y0 = x1 + (x0 - y1)*mCoeffs;            
			return y0;
		}
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				x1=x0; 
				y1=y0;
				x0=vx[n];
				y0 = x1 + (x0 - y1)*mCoeffs;            
				vy[n] = y0;
			}
			return vy;
		}
	};

	// ----------------------------------------------------------------
	// Combining the fixed delay and first order allpass section 
	// gives us an allpass-interpolated delay. In general, modulating the delay time 
	// will change the allpass coefficient, producing clicks in the output.
	
	class AllpassDelay
	{
	public:
	public:
		IntegerDelay(float d) { setDelayInSamples(d); }
		IntegerDelay() {}
		~IntegerDelay() {}
		
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
		
		inline void setDelayInSamples(float d) 
		{ 
			if(d > mBuffer.size())
			{
				setMaxDelayInSamples(d);
			}
			mIntDelayInSamples = d; 		
		}
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			return mFractionalDelay(mIntegerDelay(vx));
		}

	private:
		IntegerDelay mIntegerDelay;
		AllpassSection mFractionalDelay;
	};
	
// OLD
	
	// TODO modulating this allpass is a little bit clicky.
	// add history crossfading to address this. 
	MLSample MLAllpassDelay::processSample(const MLSample x)
	{
		float fDelayInt, D;
		float alpha, allpassIn;
		float sum;
		int delayInt;
		
		mWriteIndex &= mLengthMask;
		sum = x - mFeedback*mFixedTapOut;
		
		mBuffer[mWriteIndex] = sum;
		mWriteIndex++;
		
		// get modulation tap
		fDelayInt = floorf(mModDelayInSamples);
		delayInt = (int)fDelayInt;
		
		// get allpass interpolation coefficient D
		D = mModDelayInSamples - fDelayInt;
		
		// constrain D to [0.5 - 1.5];
		if (D < 0.5f)
		{
			D += 1.f;
			delayInt -= 1;
		}
		
		alpha = (1.f - D) / (1.f + D); // exact
		// TODO try this or Taylor approx. in van Duyne thesis
		//float xm1 = (D - 1.f);
		//alpha = -0.53f*xm1 + 0.25f*xm1*xm1; // approx on [0.5, 1.5]
		
		uintptr_t readIndex = mWriteIndex - (uintptr_t)delayInt;
		readIndex &= mLengthMask;
		allpassIn = mBuffer[readIndex];
		float modTapOut = alpha*allpassIn + mX1 - alpha*mY1;
		mX1 = allpassIn;
		mY1 = modTapOut;
		
		// get fixed tap
		readIndex = mWriteIndex - (uintptr_t)mFixedDelayInSamples;
		readIndex &= mLengthMask;
		mFixedTapOut = mBuffer[readIndex];
		
		// TODO mBlend is not dry blend, see where this is used and correct! 
		return sum*mBlend + modTapOut*mFeedForward;
	}
	

	
	
	
	// Crossfading two allpass-interpolated delays allows modulating the delay 
	// time without clicks. See "A Lossless, Click-free, Pitchbend-able Delay Line Loop Interpolation Scheme", 
	// Van Duyne, Jaffe, Scandalis, Stilson, ICMC 1997.
	
	
	class PitchbendableDelay
	{
	public:
		inline DSPVector operator()(const DSPVector vInput, const DSPVector vDelayInSamples)
		{
		}
		
	private:
		AllpassDelay mDelay1, mDelay2;
	};
	

	// ----------------------------------------------------------------
	// FDN	
	// A general Feedback Delay Network with N delay lines connected in an NxN matrix.
	
	template<int SIZE>
	class FDN
	{
	public:
		void setDelayTimesInSamples(std::array<float, SIZE> times)
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
		
		// stereo output
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
			// the identity matrix minus a constant k, where k = 2/size. Since this can be
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

		// feedback gains array is public—just copy values to set. 
		std::array<float, SIZE> mFeedbackGains{{0}};

	private:
		std::array<IntegerDelay, SIZE> mDelays;
		std::array<OnePole, SIZE> mFilters; 
		std::array<DSPVector, SIZE> mDelayInputVectors{ { {DSPVector(0.f)} } }; 
	};
	
	
	
	// ----------------------------------------------------------------
	// OverlapAdd
	
	template<int LENGTH, int DIVISIONS>
	class OverlapAdd
	{
	public:
		OverlapAdd(std::function<DSPVector(const DSPVector&)> fn, const DSPVector& w) : mFunction(fn), mWindow(w)
		{
			//mHistory.setDims(LENGTH, DIVISIONS);
		}
		~OverlapAdd(){}
		
		DSPVector operator()(const DSPVector& x)
		{
			// work in progress, use SignalBuffer?
		}
		
	private:
		//MLSignal mHistory;
		std::function<DSPVector(const DSPVector&)> mFunction;
		const DSPVector& mWindow;
	};
	
	// ----------------------------------------------------------------
	// Resampling
	
	
	// ----------------------------------------------------------------
	// Half Band Filter
	// polyphase two-path structure due to fred harris, A. G. Constantinides and Valenzuela.
	// adapted from code by Dave Waugh of Muon Software.
	// order=4, rejection=70dB, transition band=0.1
	
	class HalfBandFilter
	{
	public:
		static constexpr float ka0 = 0.07986642623635751;
		static constexpr float ka1 = 0.5453536510711322;
		static constexpr float kb0 = 0.28382934487410993;
		static constexpr float kb1 = 0.8344118914807379;
		
		inline float processSampleDown(float x1, float x2)
		{
			float y;
			a0 = apa1.processSample(apa0.processSample(x1));
			b0 = apb1.processSample(apb0.processSample(x2));
			y = (a0 + b1)*0.5f;
			b1 = b0;
			return y;
		}
		
		inline float processSampleUp(const float x)
		{
			float y;
			if(k)
			{
				a0 = apa1.processSample(apa0.processSample(x));
				y = a0;
			}
			else
			{
				b0 = apb1.processSample(apb0.processSample(x));
				y = b1;
			}
			
			b1 = b0;
			k = !k;
			return y;
		}
		
	private:
		AllpassSection apa0{ka0}, apa1{ka1}, apb0{kb0}, apb1{kb1};
		float x0{0}, x1{0};		
		float a0{0}, b0{0}, b1{0};
		bool k{false};
	};
	
	// WIP TODO multiple inputs/outputs of different sizes using variadic template maybe
	// can't use DSPVectorArray rows for multipls sources because we would like multi-row sources
	
	// what about adding INROWS, OUTROWS to template ?
	class Upsample2
	{
	public:
		Upsample2(){}
		~Upsample2(){}
		
		// upsample the input x, apply a function <DSPVector(const DSPVector&)>, downsample the result and return the output.
		// note that this could be written
		// DSPVector test1(std::function<DSPVector(const DSPVector&)> fn, const DSPVector& x)
		// but the template version allows the function parameter to be inlined.
		
		template <typename FN>
		inline DSPVector operator()(FN fn, const DSPVector& x)
		{
			
			// TODO rearrange
			
			
			// upsample to 2x buffers
			// TODO look at allpass interpolation here (see below)
			int j = 0;
			for(int i = 0; i < kFloatsPerDSPVector/2; ++i)
			{
				mUpX1[j++] = mUpper.processSampleUp(x[i]);
				mUpX1[j++] = mUpper.processSampleUp(x[i]);
			}		
			j = 0;
			for(int i = kFloatsPerDSPVector/2; i < kFloatsPerDSPVector; ++i)
			{
				mUpX2[j++] = mUpper.processSampleUp(x[i]);
				mUpX2[j++] = mUpper.processSampleUp(x[i]);
			}		
			
			// process
			mUpY1 = fn(mUpX1);
			mUpY2 = fn(mUpX2);
			
			// downsample to 1x output
			DSPVector y;
			j = 0;
			for(int i = 0; i < kFloatsPerDSPVector/2; ++i)
			{
				y[i] = mDowner.processSampleDown(mUpY1[j], mUpY1[j + 1]);
				j += 2;
			}	
			j = 0;
			for(int i = kFloatsPerDSPVector/2; i < kFloatsPerDSPVector; ++i)
			{
				y[i] = mDowner.processSampleDown(mUpY2[j], mUpY2[j + 1]);
				j += 2;
			}	
			
			return y;
		}
		
	private:
		HalfBandFilter mUpper;
		HalfBandFilter mDowner;
		DSPVector mUpX1, mUpX2, mUpY1, mUpY2;
	};
	
	
	/*
	 > ===== Fractional Delay 2X Upsampling =====
	 >
	 > Tested one more upsampling permutation, which worked the best, at 
	 > least when paired with the polyphase halfband filter. Very clean!
	 >
	 > Used JOS's simple fractional sample Allpass delay to guestimate the 
	 > intermediate samples. Something like this--
	 >
	 > //globals or class properties
	 > //Allpass delay vars
	 > double LastAPIn;
	 > double LastAPOut;
	 >
	 > //locals
	 > double tmpAPIn;
	 > double tmpAPOut;
	 > //buffer pointers
	 > float *PIndx;
	 > float *POutdx;
	 > float *PIndxTop;
	 >
	 > while (PIndx < PIndxTop) //oversample 2X
	 > {
	 >  tmpAPIn = *PIndx; //fetch insample
	 >  tmpAPOut = 0.33333333 * (tmpAPIn - LastAPOut) + LastAPIn;
	 >  //allpass delay by one-half sample
	 >  LastAPIn = tmpAPIn; //save previous values
	 >  LastAPOut = tmpAPOut;
	 >  *POutdx = tmpAPOut;
	 >  //write delay-interpolated insample to out
	 >  POutdx += 1; //inc out ptr
	 >  *POutdx = tmpAPIn;
	 >  //write original insample to out
	 >  POutdx += 1; //inc out ptr
	 >  PIndx += 1; //inc in ptr
	 > }
	 >
	 > When paired with the polyphase halfband filter, dunno why the 
	 > half-sample Allpass delay works all that much better than 
	 > zero-stuffing or repeat-sample.
	 >
	 */
	// ----------------------------------------------------------------
	// Simple time-based filters on DSPVectorArray.
	//
	/*	 
	 1 operand (filters)
	 differentiator
	 integrator
	 IntegerDelay 
	 LinearDelay
	 AllpassDelay (or, interp. set by function? allpass interp. has state. )	 
	 Downsampler2 (2n vectors -> n)
	 upsampler2 (n -> 2n) 
	 
	 overlap ( n -> 2n, 4n)
	 add ( 2, 4 -> 1)
	 
	 use templates to make different sized versions if needed so loops are still const iters
	 
	 inline DSPVector SVF::operator();
	 
	 onepole
	 asymmetriconepole
	 
	 ramp generator
	 quadratic generator
	 
	 banks:
	 ----
	 sinebank -> n vectors, templated size
	 phasebank
	 SVFbank
	 biquadbank
	 */
	
} // namespace ml

