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
	}
			
	class Biquad
	{
	public:
		Biquad() { a0 = a1 = a2 = b1 = b2 = 0.f; clear(); }
		Biquad(const MLSignal& coeffs) { setCoefficients(coeffs); clear(); }
		~Biquad() {}
		
		void clear()
		{
			x2 = x1 = y2 = y1 = 0.f;
		}
		
		void setCoefficients(float pa0, float pa1, float pa2, float pb1, float pb2)
		{
			a0 = pa0;
			a1 = pa1;
			a2 = pa2;
			b1 = pb1;
			b2 = pb2;
		}
		
		void setCoefficients(const MLSignal& coeffs)
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
			mLengthMask = (1 << mBuffer.getWidthBits() ) - 1;
			mWriteIndex = 0;
			clear();
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
		
		// MLTEST towards DSPVectors
		inline float processSample(float x)
		{
			// zero order (integer delay)
			uintptr_t readIndex = (mWriteIndex - mIntDelayInSamples) & mLengthMask;
			mWriteIndex &= mLengthMask;
			
			// uninterpolated. 
			mBuffer[mWriteIndex++] = x;			
			return mBuffer[readIndex];
		}	
		
		inline DSPVector operator()(DSPVector& x)
		{
			DSPVector y; 
	
			// TODO add wrapping / two buf concept to Signal::getWrappedRange() or something.
			
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
		FDN(int size, int delayLen)
		{ mVectorSize = 1; setMaxDelayInSamples(delayLen); resize(size); }
		~FDN(){}
		
		// size could be a template parameter with a specialization for n=4
		void resize(int n);
		void setMaxDelayInSamples(int n) { mMaxDelayLength = n; }
		void clear();
		void setDelaysInSamples(MLSignal lengths);
		void setFeedbackAmps(const MLSignal& f) { mFeedbackAmps = f; }
		void setLopass(float f);
		
		float processSample(float x);
		DSPVector operator()(DSPVector x);
		
		void setVectorSize(int v) { mVectorSize = v; }
		
	private:
		int mMaxDelayLength;
		
		std::vector<FixedDelay> mDelays;
		std::vector<Biquad> mFilters; // TODO onepole bank object
		
		MLSignal mFeedbackMatrix;
		MLSignal mDelayInputs;
		MLSignal mFeedbackAmps;
		
		std::vector<DSPVector> mDelayInputVectors;
		
		// temp?
		int mVectorSize;

	};

	// MLTEST
	MLSignal matrixMultiply2D(MLSignal A, MLSignal B);

	/*
// delay[NUM_DELAYS] is an array of my delay line class
// feedbackMatrix[NUM_DELAYS] is a float vector, used to store the feedback values between each block of samples you process

float delayOut[NUM_DELAYS];	// the size of the matrix should either be a macro, or a const int or unsigned int

for (int j=0;j<numDelays;j++)
	delay[j].Write(input + feedbackGain*feedbackMatrix[j], 0); // my delay buffers allow me to write at non-zero locations, 
								   // so I specify 0 as the delay length to write to in the second argument

for (int j=0;j<numDelays;j++)
	delayOut[j] = delay[j].Read(delayLength[j]);

for (int j=0;j<numDelays;j++)
	delayOut[j] = filter[j].Process(delayOut[j]);

float sum = 0.f; // you need to declare this for every sample/block, or at least zero it out

for (int j=0;j<numDelays;j++)
	sum += delayOut[j];

sum *= 2.f/numDelays;

for (int j=0;j<numDelays;j++)
	feedbackMatrix[j] = delayOut[j] - sum;

for (int j=0;j<numDelays;j++)
	delay[j].UpdateBasePointer();		// I can have a lot of read/write operations per delay, so I increment the base pointers manually

For a 4x4 FDN, you get a perfectly diffuse matrix. Larger matrices start becoming less and less diffuse, in that most of the feedback per delay comes from that same delay’s output. Permuting the matrix in such a case is useful, so that most of the output of the delay feeds the NEXT delay - kind of like an allpass loop.

For larger FDNs, you can build up Hadamard matrices out of 2x2 rotation matrices. This is a very nice technique, that Miller Puckette covers in his work. 

The pseudocode above works for block based processing as well. If you are working with 1 big block of memory (instead of separate delay buffers), make sure that you have enough room in there for the block size. I have done block based processing with a single delay buffer in the past, but it is really a pain. I’d suggest working with separate delay buffers in this case. The matrix adds/subtracts can be done in block base processing, where the blocks span time (i.e. samples in series) versus parallel delay branches. SSE for parallel delay branches would be useful for the filtering, but I haven’t done this - I tend to SIMD for series samples.

Hope this helps. Feel free to ask any follow up questions.

Sean
	*/
	
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