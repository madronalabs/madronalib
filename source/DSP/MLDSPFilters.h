//
// MLDSPFilters.h
// madronalib
//
// Created by Randy Jones on 4/14/2016
//
// DSP filters: functor objects implementing an inline DSPVector operator()(DSPVector input),
// in order to make filters. All these filters have some state, otherwise they would be DSPOps.
// 
// These objects are for building fixed DSP graphs in a functional style. The compiler should 
// have many opportunities to optimize these graphs. For dynamic graphs changeable at runtime,
// see MLProcs. In general MLProcs will be written using DSPGens, DSPOps, DSPFilters.

#pragma once

#include "MLDSPOps.h"
#include "MLDSPGens.h" // MLTEST
#include "MLSignal.h"
#include "MLProperty.h"

namespace ml
{
	namespace onePoleCoeffs
	{
		inline MLSignal passthru()
		{
			return MLSignal{1.f, 0.f};
		}
		
		//TODO this can just return a float
		inline MLSignal onePole(float omega)
		{
			float x = expf(-omega);
			float a0 = 1.f - x;
			float b1 = -x;
			return MLSignal{a0, -b1};
		}
	}
	
	namespace biquadCoeffs
	{
		inline MLSignal passthru()
		{
			return MLSignal{1.f, 0.f, 0.f, 0.f, 0.f};
		}
				
		inline MLSignal lopass(float omega, float q)
		{
			//LPF:        H(s) = 1 / (s^2 + s/Q + 1)
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float b0 = 1.f / (1.f + alpha);
			
			float a0 = (1.f - cosOmega) * 0.5f * b0;
			float a1 = (1.f - cosOmega) * b0;
			float a2 = (1.f - cosOmega) * 0.5f * b0;
			float b1 = -2.f * cosOmega * b0;
			float b2 = (1.f - alpha) * b0;
			return MLSignal{a0, a1, a2, -b1, -b2};
		}
		
		inline MLSignal hipass(float omega, float q)
		{
			//HPF:        H(s) = s^2 / (s^2 + s/Q + 1)
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float b0 = 1.0f / (1.f + alpha);
			
			float a0 = (1.f + cosOmega) * 0.5f *b0;
			float a1 = -(1.f + cosOmega) *b0;
			float a2 = (1.f + cosOmega) * 0.5f *b0;
			float b1 = -2.f * cosOmega *b0;
			float b2 = (1.f - alpha) *b0;
			return MLSignal{a0, a1, a2, -b1, -b2};
		}
		
		inline MLSignal peakNotch(float omega, float q, float gain)
		{
			//notch: H(s) = (s^2 + 1) / (s^2 + s/Q + 1)
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float A = sqrtf(gain);
			float alphaOverA = alpha/A;
			A *= alpha;
			float b0 = 1.f / (1.f + alphaOverA);
			
			float a0 = (1.f + A) * b0;
			float a1 = -2.f * cosOmega * b0;
			float a2 = (1.f - A) * b0;
			float b1 = a1*b0;
			float b2 = (1.f - alphaOverA) * b0;
			return MLSignal{a0, a1, a2, -b1, -b2};
		}
		
		inline MLSignal bandpass(float omega, float q)
		{
			//BPF: H(s) = s / (s^2 + s/Q + 1)  (constant skirt gain, peak gain = Q)
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float b0 = 1.f + alpha;
			
			float a0 = alpha / b0;
			float a1 = 0.;
			float a2 = -alpha / b0;
			float b1 = -2.f * cosOmega / b0;
			float b2 = (1.f - alpha) / b0;
			return MLSignal{a0, a1, a2, -b1, -b2};
		}
		
		inline MLSignal onePole(float omega)
		{
			float x = expf(-omega);
			float a0 = 1.f - x;
			float a1 = 0.f;
			float a2 = 0.f;
			float b1 = -x;
			float b2 = 0.f;
			return MLSignal{a0, a1, a2, -b1, -b2};
		}
			
		inline MLSignal loShelf(float omega, float q, float gain)
		{
			// lowShelf: H(s) = A * (s^2 + (sqrt(A)/Q)*s + A)/(A*s^2 + (sqrt(A)/Q)*s + 1)
			float A = gain;
			float aMinus1 = A - 1.0f;
			float aPlus1 = A + 1.0f;
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float beta = 2.0f*sqrtf(A)*alpha;
			
			float b0 = aPlus1 + aMinus1*cosOmega + beta;			
			float a0 = (A*(aPlus1 - aMinus1*cosOmega + beta)) / b0;
			float a1 = (A*(aPlus1*-2.0f*cosOmega + 2.0f*aMinus1)) / b0;
			float a2 = (A*(aPlus1 - aMinus1*cosOmega - beta)) / b0;
			float b1 = (aPlus1*-2.0f*cosOmega - 2.0f*aMinus1) / b0;
			float b2 = (aPlus1 + aMinus1*cosOmega - beta) / b0;
			
			return MLSignal{a0, a1, a2, -b1, -b2};
		}
		
		inline MLSignal hiShelf(float omega, float q, float gain)
		{
			// highShelf: H(s) = A * (A*s^2 + (sqrt(A)/Q)*s + 1)/(s^2 + (sqrt(A)/Q)*s + A)
			float A = gain;
			float aMinus1 = A - 1.0f;
			float aPlus1 = A + 1.0f;
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float beta = 2.0f*sqrtf(A)*alpha;
			
			float b0 = aPlus1 - aMinus1*cosOmega + beta;			
			float a0 = (A*(aPlus1 + aMinus1*cosOmega + beta)) / b0;
			float a1 = (A*(aPlus1*-2.0f*cosOmega + -2.0f*aMinus1)) / b0;
			float a2 = (A*(aPlus1 + aMinus1*cosOmega - beta)) / b0;
			float b1 = (aPlus1*-2.0f*cosOmega + 2.0f*aMinus1) / b0;
			float b2 = (aPlus1 - aMinus1*cosOmega - beta) / b0;
			
			return MLSignal{a0, a1, a2, -b1, -b2};
		}
		
		inline MLSignal multiplyGain(MLSignal xc, float g)
		{
			MLSignal yc = xc;
			yc[0] *= g;
			return yc;
		}
	}
	
	// TODO : time-carying coeffs generating DSPVectorArray<5>
	namespace biquadCoeffsVarying
	{
		inline DSPVectorArray<5> lopass(const DSPVector& omega, const DSPVector& q)
		{
			/*
			//LPF:        H(s) = 1 / (s^2 + s/Q + 1)
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float b0 = 1.f / (1.f + alpha);
			
			float a0 = (1.f - cosOmega) * 0.5f * b0;
			float a1 = (1.f - cosOmega) * b0;
			float a2 = (1.f - cosOmega) * 0.5f * b0;
			float b1 = -2.f * cosOmega * b0;
			float b2 = (1.f - alpha) * b0;
			return MLSignal{a0, a1, a2, -b1, -b2};
			*/
			return DSPVectorArray<5>();
		}
	}
	
	class OnePole
	{
	public:
		OnePole() { setCoeffs(onePoleCoeffs::passthru()); clear(); }
		
		OnePole(const MLSignal& coeffs) { setCoeffs(coeffs); clear(); }
		~OnePole() {}
		
		void clear()
		{
			y1 = 0.f;
		}
		
		void setCoeffs(const MLSignal& coeffs)
		{
			a0 = coeffs[0];
			b1 = coeffs[1];
		}
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float fx = vx[n];
				const float fy = a0*fx + b1*y1;
				y1 = fy;
				vy[n] = fy;
			}
			return vy;
		}
		
		inline DSPVector operator()(const DSPVector& vx, const DSPVectorArray<2>& vc)
		{
			DSPVector vy;
			const float * pA0 = vc.getRowDataConst<0>();
			const float * pB1 = vc.getRowDataConst<1>();
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float fx = vx[n];
				const float fy = pA0[n]*fx + pB1[n]*y1;
				y1 = fy;
				vy[n] = fy;
			}
			return vy;
		}
		
	private:
		float a0, b1;
		float y1;
	};

	class DCBlocker
	{
	public:
		DCBlocker() { mR = 0.999f; clear(); }
		
		void clear()
		{
			x1 = y1 = 0.f;
		}
		
		void setOmega(float omega)
		{
			mR = cosf(omega);
		}
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				const float fx = vx[n];
				const float fy = fx - x1 + mR*y1;
				y1 = fy;
				x1 = fx;
				vy[n] = fy;
			}
			return vy;
		}

	private:
		float mR;
		float x1, y1;
	};

	class Biquad
	{
	public:
		// any kind of filter that can be made with a default constructor should default to a passthru.
		
		// TODO filter coeffs should not be signals, rather custom types! 
		Biquad() { setCoeffs(biquadCoeffs::passthru()); clear(); }
		
		Biquad(const MLSignal& coeffs) { setCoeffs(coeffs); clear(); }
		~Biquad() {}
		
		void clear()
		{
			x2 = x1 = y2 = y1 = 0.f;
		}
		
		void setCoeffs(const MLSignal& coeffs)
		{
			a0 = coeffs[0];
			a1 = coeffs[1];
			a2 = coeffs[2];
			b1 = coeffs[3];
			b2 = coeffs[4];
		}
		
		// process one DSPVector of data with constant coefficients. 
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				const float fx = vx[n];				
				const float fy = a0*fx + a1*x1 + a2*x2 + b1*y1 + b2*y2;
				x2 = x1; x1 = fx; 
				y2 = y1; y1 = fy;				
				vy[n] = fy;
			}		
			return vy;
		}
		
		// process one DSPVector of data with time-varying coefficients. 
		inline DSPVector operator()(const DSPVector& vx, const DSPVectorArray<5>& vc)
		{
			DSPVector vy;
			const float * pA0 = vc.getRowDataConst<0>();
			const float * pA1 = vc.getRowDataConst<1>();
			const float * pA2 = vc.getRowDataConst<2>();
			const float * pB1 = vc.getRowDataConst<3>();
			const float * pB2 = vc.getRowDataConst<4>();
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float fx = vx[n];				
				const float fy = pA0[n]*fx + pA1[n]*x1 + pA2[n]*x2 + pB1[n]*y1 + pB2[n]*y2;
				x2 = x1; x1 = fx; 
				y2 = y1; y1 = fy;				
				vy[n] = fy;
			}		
			return vy;
		}
		
	private:
		float a0, a1, a2, b1, b2;
		float x1, x2, y1, y2;
	};
	
// --------------------------------------------------------------------------------
// SVF variations
// Thanks to Andrew Simper [www.cytomic.com] for sharing his work.
	
	// TODO: time-varying coefficients, time-varying operators for each SVF type
	
	namespace svfCoeffs
	{
		inline MLSignal silent()
		{
			return MLSignal{0.f, 0.f, 0.f};
		}
		
		// get SVF coefficients, which are the same for all modes at the given frequency and q.
		// k = 1/q (damping factor). 
		//
		// NOTE: omega is defined here as cutoff / sr, not the more usual 2pi*cutoff/sr.
		//
		inline MLSignal atOmegaAndK(float omega, float k)
		{
			float piOmega = kPi*omega;
			float s1 = sinf(piOmega);
			float s2 = sinf(2.0f*piOmega);
			float nrm = 1.0f/(2.f + k*s2);
			float g0 = s2*nrm;
			float g1 = (-2.f*s1*s1 - k*s2)*nrm;
			float g2 = (2.0f*s1*s1)*nrm;			
			return MLSignal{g0, g1, g2, k};
		}
	}
	
	class SVFBandpass
	{
	public:
		SVFBandpass() { setCoeffs(svfCoeffs::silent()); clear(); }
		void clear() { ic1eq = ic2eq = 0.f; }
		void setCoeffs(const MLSignal& coeffs) { g0 = coeffs[0]; g1 = coeffs[1]; g2 = coeffs[2]; }
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float t0 = v0 - ic2eq;
				float t1 = g0*t0 + g1*ic1eq;
				float t2 = g2*t0 + g0*ic1eq;
				float v1 = t1 + ic1eq;
				//			float v2 = t2 + ic2eq; // for hipass
				ic1eq += 2.0f*t1;
				ic2eq += 2.0f*t2;
				vy[n] = v1; 
			}		
			return vy;
		}
		
		float g0, g1, g2;
		float ic1eq, ic2eq;
	};
	
	class SVFLopass
	{
	public:
		SVFLopass() { setCoeffs(svfCoeffs::silent()); clear(); }
		void clear() { ic1eq = ic2eq = 0.f; }
		void setCoeffs(const MLSignal& coeffs) { g0 = coeffs[0]; g1 = coeffs[1]; g2 = coeffs[2]; }
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float t0 = v0 - ic2eq;
				float t1 = g0*t0 + g1*ic1eq;
				float t2 = g2*t0 + g0*ic1eq;
				float v2 = t2 + ic2eq;
				ic1eq += 2.0f*t1;
				ic2eq += 2.0f*t2;
				vy[n] = v2;
			}
			return vy;
		}
		
		float g0, g1, g2;
		float ic1eq, ic2eq;
	};
	
	class SVFHipass
	{
	public:
		SVFHipass() { setCoeffs(svfCoeffs::silent()); clear(); }
		void clear() { ic1eq = ic2eq = 0.f; }
		void setCoeffs(const MLSignal& coeffs) { g0 = coeffs[0]; g1 = coeffs[1]; g2 = coeffs[2]; k = coeffs[3]; }
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float t0 = v0 - ic2eq;
				float t1 = g0*t0 + g1*ic1eq;
				float t2 = g2*t0 + g0*ic1eq;
				float v1 = t1 + ic1eq;
				float v2 = t2 + ic2eq;
				ic1eq += 2.0f*t1;
				ic2eq += 2.0f*t2;
				vy[n] = v0 - k*v1 - v2; 
			}		
			return vy;
		}
		
		float g0, g1, g2, k;
		float ic1eq, ic2eq;
	};
	
	class SVFPeak
	{
	public:
		SVFPeak() { setCoeffs(svfCoeffs::silent()); clear(); }
		void clear() { ic1eq = ic2eq = 0.f; }
		void setCoeffs(const MLSignal& coeffs) { g0 = coeffs[0]; g1 = coeffs[1]; g2 = coeffs[2]; k = coeffs[3]; }
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float t0 = v0 - ic2eq;
				float t1 = g0*t0 + g1*ic1eq;
				float t2 = g2*t0 + g0*ic1eq;
				float v1 = t1 + ic1eq;
				float v2 = t2 + ic2eq;
				ic1eq += 2.0f*t1;
				ic2eq += 2.0f*t2;
				vy[n] = v0 - k*v1 - 2*v2; 
			}		
			return vy;
		}
		
		float g0, g1, g2, k;
		float ic1eq, ic2eq;
	};

	class SVFNotch
	{
	public:
		SVFNotch() { setCoeffs(svfCoeffs::silent()); clear(); }
		void clear() { ic1eq = ic2eq = 0.f; }
		void setCoeffs(const MLSignal& coeffs) { g0 = coeffs[0]; g1 = coeffs[1]; g2 = coeffs[2]; k = coeffs[3]; }
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float v0 = vx[n];
				float t0 = v0 - ic2eq;
				float t1 = g0*t0 + g1*ic1eq;
				float t2 = g2*t0 + g0*ic1eq;
				float v1 = t1 + ic1eq;
				ic1eq += 2.0f*t1;
				ic2eq += 2.0f*t2;
				vy[n] = v0 - k*v1; 
			}		
			return vy;
		}
		
		float g0, g1, g2, k;
		float ic1eq, ic2eq;
	};

	// TODO SVF Lo shelf, hi shelf
	
// --------------------------------------------------------------------------------
#pragma mark fixed delay
	
	// a simple uninterpolated delay with no feedback.
	class FixedDelay
	{
	public:
		FixedDelay(int d) { setMaxDelayInSamples(d); setDelayInSamples(d); }
		FixedDelay() {}
		~FixedDelay() {}
		
		void setMaxDelayInSamples(int dMax)
		{
			mBuffer.setDims(dMax + kFloatsPerDSPVector);
			mLengthMask = (1 << mBuffer.getWidthBits() ) - 1; // TODO MLSignal::getLengthMask?
			mWriteIndex = 0;
			clear();
		}
		int getMaxDelayInSamples()
		{
			return (1 << mBuffer.getWidthBits() ) - 1;
		}
		
		inline void clear()
		{
			mBuffer.clear();
		}
		
		inline void setDelayInSamples(int d) 
		{ 
			if(d > getMaxDelayInSamples())
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
				std::copy(srcStart, srcStart + kFloatsPerDSPVector, mBuffer.getBuffer() + mWriteIndex);
			}
			else
			{
				uintptr_t excess = writeEnd - mLengthMask - 1; 
				const float* srcStart = x.getConstBuffer();
				const float* srcSplice = srcStart + kFloatsPerDSPVector - excess;
				const float* srcEnd = srcStart + kFloatsPerDSPVector;
				std::copy(srcStart, srcSplice, mBuffer.getBuffer() + mWriteIndex);
				std::copy(srcSplice, srcEnd, mBuffer.getBuffer());
			}
			
			// read			
			DSPVector y; 
			uintptr_t readStart = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
			uintptr_t readEnd = readStart + kFloatsPerDSPVector;
			float* srcBuf = mBuffer.getBuffer();
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
		MLSignal mBuffer;
		int mIntDelayInSamples;
		uintptr_t mWriteIndex;
		uintptr_t mLengthMask;
	};
	
	
	// TODO crossfading allpass delay as described at ICMC97 by Van Duyne et al
	// class AllpassDelay
	 
// ----------------------------------------------------------------
#pragma mark FDN
	
	// A general Feedback Delay Network with N delay lines connected in an NxN matrix.
	class FDN
	{
	public:
		FDN(std::initializer_list<MLPropertyChange> p);
		~FDN(){}

		void setProperty(Symbol name, MLProperty value);
		
		void clear();
		
		// stereo output
		DSPVectorArray<2> operator()(const DSPVector& x);
		
	private:
		std::vector<FixedDelay> mDelays;
		std::vector<Biquad> mFilters; // TODO onepole bank object		
		std::vector<DSPVector> mDelayInputVectors;
		MLSignal mFeedbackGains;
	};
	
	
	// ----------------------------------------------------------------
#pragma mark OverlapAdd
	
	template<int LENGTH, int DIVISIONS>
	class OverlapAdd
	{
	public:
		OverlapAdd(std::function<DSPVector(const DSPVector&)> fn, const DSPVector& w) : mFunction(fn), mWindow(w)
		{
			mHistory.setDims(LENGTH, DIVISIONS);
		}
		~OverlapAdd(){}
		
		DSPVector operator()(const DSPVector& x)
		{
			// work in progress, use SignalBuffer?
		}
		
	private:
		MLSignal mHistory;
		std::function<DSPVector(const DSPVector&)> mFunction;
		const DSPVector& mWindow;
	};
	
	// ----------------------------------------------------------------
	#pragma mark Resampling
	

	
	// ----------------------------------------------------------------
	#pragma mark HalfBandFilter
	
	class HalfBandFilter
	{
	public:
		static const float ka0, ka1, kb0, kb1;
		
		class AllpassSection
		{
		public:
			AllpassSection();
			~AllpassSection();
			void clear();
			
			inline float processSample(const float x)
			{
				x1=x0;
				y1=y0;
				x0=x;
				y0 = x1 + (x0 - y1)*a;            
				return y0;
			}
			
			float x0, x1, y0, y1;
			float a;        
		};
		
		HalfBandFilter();
		~HalfBandFilter();
		void clear();
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
		AllpassSection apa0, apa1, apb0, apb1;
		float x0, x1;
		float a0, b0, b1;
		bool k;
	};
	
	
	
	inline DSPVector half1Up2(const DSPVector& x)
	{
		DSPVector y;
		// TODO SSE
		int j = 0;
		for(int i=0; i<kFloatsPerDSPVector/2; ++i)
		{
			y[j++] = x[i];
			y[j++] = x[i];		
		}
		return y;
	}
	
	inline DSPVector half2Up2(const DSPVector& x)
	{
		DSPVector y;
		// TODO SSE
		int j = 0;
		for(int i=kFloatsPerDSPVector/2; i<kFloatsPerDSPVector; ++i)
		{
			y[j++] = x[i];
			y[j++] = x[i];		
		}
		return y;
	}

	// WIP TODO multiple params using vriadic template maybe
	// can't use rows because we woud like multi-row params
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
			// up to 2x buffers
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
			
			// call fn (would in place be OK?)
			mUpY1 = fn(mUpX1);
			mUpY2 = fn(mUpX2);
			
			// down to 1x output
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
	

	// ----------------------------------------------------------------
	// Simple time-based filters on DSPVectorArray.
	//
	/*	 
		 1 operand (filters)
		 differentiator
		 integrator
		 FixedDelay 
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
