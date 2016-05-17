//
//  MLDSPFilters.h
//  madronalib
//
//  Created by Randy Jones on 4/14/2016
//
//

#pragma once

#include "MLDSPOps.h"
#include "MLSignal.h"

namespace ml
{
	namespace onePoleCoeffs
	{
		inline MLSignal passthru()
		{
			return MLSignal{1.f, 0.f};
		}
		
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
	
	namespace svfCoeffs
	{
		inline MLSignal passthru()
		{
			return MLSignal{1.f, 0.f, 0.f};
		}
		
		// get SVF coefficients, which are the same for all modes at the given frequency and q.
		// k = 1/q (damping factor).
		inline MLSignal atOmegaAndK(float omega, float k)
		{
			float s1 = sinf(omega);
			float s2 = sinf(2.0f*omega);
			float nrm = 1.0f/(2.f + k*s2);
			float g0 = s2*nrm;
			float g1 = (-2.f*s1*s1 - k*s2)*nrm;
			float g2 = (2.0f*s1*s1)*nrm;
			
			return MLSignal{g0, g1, g2, k};
			
			// tried fast sin approx, which did not hold up. try better approximations.		
		}
	}
			
	class OnePole
	{
	public:
		// any kind of filter that can be made with a default constructor should default to a passthru.
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

	class Biquad
	{
	public:
		// any kind of filter that can be made with a default constructor should default to a passthru.
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
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector vy;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				float fx = vx[n];				
				const float fy = a0*fx + a1*x1 + a2*x2 + b1*y1 + b2*y2;
				x2 = x1; x1 = fx; y2 = y1; y1 = fy;				
				vy[n] = fy;
			}		
			return vy;
		}
		
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
				x2 = x1; x1 = fx; y2 = y1; y1 = fy;				
				vy[n] = fy;
			}		
			return vy;
		}
		
	private:
		float a0, a1, a2, b1, b2;
		float x1, x2, y1, y2;
	};
	
// --------------------------------------------------------------------------------
#pragma mark SVF variations
	
	class SVFBandpass
	{
	public:
		// any kind of filter that can be made with a default constructor should default to a passthru.
		SVFBandpass() { setCoeffs(svfCoeffs::passthru()); clear(); }
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
		// any kind of filter that can be made with a default constructor should default to a passthru.
		SVFLopass() { setCoeffs(svfCoeffs::passthru()); clear(); }
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
		// any kind of filter that can be made with a default constructor should default to a passthru.
		SVFHipass() { setCoeffs(svfCoeffs::passthru()); clear(); }
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
		SVFPeak() { setCoeffs(svfCoeffs::passthru()); clear(); }
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
		SVFNotch() { setCoeffs(svfCoeffs::passthru()); clear(); }
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
			DSPVector y; 

			// write
			uintptr_t writeEnd = mWriteIndex + kFloatsPerDSPVector;
			if(writeEnd <= mLengthMask + 1)
			{
				std::copy(x.mData.asFloat, x.mData.asFloat + kFloatsPerDSPVector, mBuffer.getBuffer() + mWriteIndex);
			}
			else
			{
				uintptr_t excess = writeEnd - mLengthMask - 1; 
				float* srcStart = x.mData.asFloat;
				float* srcSplice = srcStart + kFloatsPerDSPVector - excess;
				float* srcEnd = srcStart + kFloatsPerDSPVector;
				std::copy(srcStart, srcSplice, mBuffer.getBuffer() + mWriteIndex);
				std::copy(srcSplice, srcEnd, mBuffer.getBuffer());
			}
			
			// read			
			uintptr_t readStart = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
			uintptr_t readEnd = readStart + kFloatsPerDSPVector;
			float* srcBuf = mBuffer.getBuffer();
			if(readEnd <= mLengthMask + 1)
			{
				std::copy(srcBuf + readStart, srcBuf + readEnd, y.mData.asFloat);
			}
			else
			{
				uintptr_t excess = readEnd - mLengthMask - 1; 
				uintptr_t readSplice = readStart + kFloatsPerDSPVector - excess;
				std::copy(srcBuf + readStart, srcBuf + readSplice, y.mData.asFloat);
				std::copy(srcBuf, srcBuf + excess, y.mData.asFloat + (kFloatsPerDSPVector - excess));
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
		FDN(MLSignal delayLengths){ setDelaysInSamples(delayLengths); }
		~FDN(){}
		
		// set delay times in samples, resizing delays if needed.
		void setDelaysInSamples(MLSignal lengths);
		
		// set filter cutoffs without resizing.
		// FDN does not know its sample rate, so filter cutoffs are set as radial frequencies
		void setFilterCutoffs(MLSignal filterCutoffs);
		
		void setFeedbackGains(MLSignal gains);
		
		void clear();
		
		// stereo output
		DSPVectorArray<2> operator()(DSPVector x);
		
	private:
		std::vector<FixedDelay> mDelays;
		std::vector<Biquad> mFilters; // TODO onepole bank object		
		std::vector<DSPVector> mDelayInputVectors;
		std::vector<float> mFeedbackGains;
	};
	
	// ----------------------------------------------------------------
	#pragma mark OverlapAdd
	
	template<int LENGTH, int DIVISIONS>
	class OverlapAdd
	{
	public:
		OverlapAdd(std::function<DSPVector(DSPVector)> fn, const DSPVector& w) : mFunction(fn), mWindow(w) 
		{
			mHistory.setDims(LENGTH, DIVISIONS);
		}
		~OverlapAdd(){}
		
		DSPVector operator()(DSPVector x)
		{
			
		}
		
	private:
		MLSignal mHistory;
		std::function<DSPVector(DSPVector)> mFunction;
		const DSPVector& mWindow;
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