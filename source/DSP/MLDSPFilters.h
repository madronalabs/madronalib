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
	namespace biquadCoeffs
	{
		inline MLSignal passthru()
		{
			float a0 = 1.f;
			float a1 = 0.f;
			float a2 = 0.f;
			float b1 = 0.f;
			float b2 = 0.f;
			return MLSignal{a0, a1, a2, b1, b2};
		}
		
		inline MLSignal onePole(float omega)
		{
			float x = expf(-omega);
			float a0 = 1.f - x;
			float a1 = 0.f;
			float a2 = 0.f;
			float b1 = -x;
			float b2 = 0.f;
			return MLSignal{a0, a1, a2, b1, b2};
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
			
			return MLSignal{a0, a1, a2, b1, b2};
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
			
			return MLSignal{a0, a1, a2, b1, b2};
		}
		
		inline MLSignal multiplyGain(MLSignal xc, float g)
		{
			MLSignal yc = xc;
			yc[0] *= g;
			return yc;
		}
	}
			
	class Biquad
	{
	public:
		// any filter that can be made with a default constructor should default to passthru.
		Biquad() { setCoeffs(biquadCoeffs::passthru()); clear(); }
		
		Biquad(const MLSignal& coeffs) { setCoeffs(coeffs); clear(); }
		~Biquad() {}
		
		void clear()
		{
			x2 = x1 = y2 = y1 = 0.f;
		}
		
		void setCoeffs(float pa0, float pa1, float pa2, float pb1, float pb2)
		{
			a0 = pa0;
			a1 = pa1;
			a2 = pa2;
			b1 = pb1;
			b2 = pb2;
		}
		
		void setCoeffs(const MLSignal& coeffs)
		{
			a0 = coeffs[0];
			a1 = coeffs[1];
			a2 = coeffs[2];
			b1 = coeffs[3];
			b2 = coeffs[4];
		}
		
		inline float processSample(float x)
		{
			const float out = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
			x2 = x1;
			x1 = x;
			y2 = y1;
			y1 = out;
			return(out);
		}
		
		inline DSPVector operator()(const DSPVector& vx)
		{
			DSPVector y;
			for(int n=0; n<kFloatsPerDSPVector; ++n)
			{
				const float x = vx[n];
				const float out = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
				x2 = x1;
				x1 = x;
				y2 = y1;
				y1 = out;
				y[n] = out;
			}		
			return y;
		}
		
		float a0, a1, a2, b1, b2;
		float x1, x2, y1, y2;
	};
	
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
			int delay = clamp(d, 1, mBuffer.getWidth());
			mIntDelayInSamples = (delay); 		
		}
		
		inline DSPVector operator()(DSPVector& x)
		{
			DSPVector y; 

			// TODO add wrapping / two buf concept to Signal::getWrappedRange() or something.
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

		// clear state.
		void clear();
		
		DSPVector operator()(DSPVector x);
		
	private:
		std::vector<FixedDelay> mDelays;
		std::vector<Biquad> mFilters; // TODO onepole bank object		
		std::vector<DSPVector> mDelayInputVectors;
		std::vector<float> mFeedbackGains;
	};

	// ----------------------------------------------------------------
	// Simple time-based filters on DSPVectors.
	//
	/*	 
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
	
} // namespace ml