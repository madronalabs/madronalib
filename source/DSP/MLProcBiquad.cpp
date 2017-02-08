
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

const float kLowFrequencyLimit = 70.f; 

// ----------------------------------------------------------------
// class definition

class MLProcBiquad : public MLProc
{
public:
	 MLProcBiquad();
	~MLProcBiquad();
	
	err resize() override;
	void clear() override;
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	typedef enum 
	{
		kLowpass = 0,
		kHighpass = 1,
		kBandpass = 2,
		kNotch = 3,
		kLoShelf = 4,
		kHiShelf = 5
	}	eBiquadMode;
	
	MLProcInfo<MLProcBiquad> mInfo;
	void calcCoeffs(const int n);
	
	// coeff signals
	MLSignal mA0, mA1, mA2, mB1, mB2;
	
	// saved constant coeffs
	MLSample mCFrequency, mCQ;
	
	// history
	MLSample mX1, mX2, mY1, mY2;

};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcBiquad> classReg("biquad");
	ML_UNUSED MLProcParam<MLProcBiquad> params[1] = {"mode"};
	ML_UNUSED MLProcInput<MLProcBiquad> inputs[] = {"in", "frequency", "q", "gain"}; 
	ML_UNUSED MLProcOutput<MLProcBiquad> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

MLProcBiquad::MLProcBiquad()
{
}

MLProcBiquad::~MLProcBiquad()
{
}

// set up coefficient signals
MLProc::err MLProcBiquad::resize() 
{	
	MLProc::err e = OK;
	int b = getContextVectorSize();
	mA0.setDims(b);
	mA1.setDims(b);
	mA2.setDims(b);
	mB1.setDims(b);
	mB2.setDims(b);
	
	// TODO check
	return e;
}

void MLProcBiquad::clear() 
{	
	mCFrequency = mCQ = 0.f;
	mX1 = mX2 = mY1 = mY2 = 0.f;
}

static ml::Symbol modeSym("mode");

void MLProcBiquad::calcCoeffs(const int frames) 
{
	int mode = (int)getParam(modeSym);
	const MLSignal& frequency = getInput(2);
	const MLSignal& q = getInput(3);
	const MLSignal& gain = getInput(4);
	int coeffFrames = frames;
	
	float twoPiOverSr = kMLTwoPi*getContextInvSampleRate();		
	
	float a0, a1, a2, b0, b1, b2;
	float qm1, omega, alpha, sinOmega, cosOmega;
	float highLimit = getContextSampleRate() * 0.33f;
				
	// generate coefficient signals
	// TODO SSE --  use the DSP Utils library! 
	for(int n=0; n<coeffFrames; ++n)
	{
		qm1 = 1.f/(q[n] + 0.05f);
		omega = ml::clamp(frequency[n], kLowFrequencyLimit, highLimit) * twoPiOverSr;
		sinOmega = fsin1(omega);
		cosOmega = fcos1(omega);
		alpha = sinOmega * 0.5f * qm1;
		b0 = 1.f/(1.f + alpha);
				
		switch (mode) 
		{
		default:
		case kLowpass:
			a0 = ((1.f - cosOmega) * 0.5f)*b0;
			a1 = (1.f - cosOmega)*b0;
			a2 = a0;
			b1 = (-2.f * cosOmega)*b0;
			b2 = (1.f - alpha)*b0;		
			break;
				
		case kHighpass:		
			a0 = ((1.f + cosOmega) * 0.5f)*b0;
			a1 = -(1.f + cosOmega)*b0;
			a2 = a0;
			b1 = (-2.f * cosOmega)*b0;
			b2 = (1.f - alpha)*b0;
			break;
								
		case kBandpass:
			a0 = alpha*b0;
			a1 = 0.f*b0;
			a2 = -alpha*b0;
			b1 = -2.f * cosOmega*b0;
			b2 = (1.f - alpha)*b0;
			break;
			
		case kNotch:
			a0 = 1*b0;
			a1 = -2.f * cosOmega*b0;
			a2 = 1*b0;
			b1 = -2.f * cosOmega*b0;
			b2 = (1.f - alpha)*b0;
			break;
				
			case kLoShelf:
			{
				float A = gain[n];
				float aMinus1 = A - 1.0f;
				float aPlus1 = A + 1.0f;
				float alpha2 = sinOmega / (2.f * q[n]);
				float beta = 2.0f*sqrtf(A)*alpha2;
				float b00 = 1.0f/(aPlus1 + aMinus1*cosOmega + beta);
				
				a0 = (A*(aPlus1 - aMinus1*cosOmega + beta))*b00;
				a1 = (A*(aPlus1*-2.0f*cosOmega + 2.0f*aMinus1))*b00;
				a2 = (A*(aPlus1 - aMinus1*cosOmega - beta))*b00;
				b1 = (aPlus1*-2.0f*cosOmega - 2.0f*aMinus1)*b00;
				b2 = (aPlus1 + aMinus1*cosOmega - beta)*b00;
			}
			break;
				
			case kHiShelf:
			{
				float A = gain[n];
				float aMinus1 = A - 1.0f;
				float aPlus1 = A + 1.0f;
				float alpha2 = sinOmega / (2.f * q[n]);
				float beta = 2.0f*sqrtf(A)*alpha2;
				float b00 = 1.0f/(aPlus1 + aMinus1*cosOmega + beta);

				a0 = (A*(aPlus1 + aMinus1*cosOmega + beta))*b00;
				a1 = (A*(aPlus1*-2.0f*cosOmega - 2.0f*aMinus1))*b00;
				a2 = (A*(aPlus1 + aMinus1*cosOmega - beta))*b00;
				b1 = (aPlus1*-2.0f*cosOmega + 2.0f*aMinus1)*b00;
				b2 = (aPlus1 - aMinus1*cosOmega - beta)*b00;
			}

			break;
		}
						
		mA0[n] = a0;
		mA1[n] = a1;
		mA2[n] = a2;
		mB1[n] = b1;
		mB2[n] = b2;
	}
}


void MLProcBiquad::process()
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	
	calcCoeffs(kFloatsPerDSPVector);
	
	for (int n=0; n<kFloatsPerDSPVector; ++n)
	{
		float in, out;
		in = x[n];
		out = mA0[n]*in + mA1[n]*mX1 + mA2[n]*mX2 - mB1[n]*mY1 - mB2[n]*mY2;
		mX2 = mX1;
		mX1 = in;
		mY2 = mY1;
		mY1 = out;
		y[n] = out;
	}
}
	   
