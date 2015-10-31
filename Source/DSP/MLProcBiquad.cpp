
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
	
	err resize();
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	typedef enum 
	{
		kLowpass = 0,
		kHighpass,
		kBandpass,
		kNotch
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
	ML_UNUSED MLProcInput<MLProcBiquad> inputs[] = {"in", "frequency", "q"}; 
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

void MLProcBiquad::calcCoeffs(const int frames) 
{
	static MLSymbol modeSym("mode");
	int mode = (int)getParam(modeSym);
	const MLSignal& frequency = getInput(2);
	const MLSignal& q = getInput(3);
	int coeffFrames;
	
	float twoPiOverSr = kMLTwoPi*getContextInvSampleRate();		

	bool paramSignalsAreConstant = frequency.isConstant() && q.isConstant();
	
	if (paramSignalsAreConstant)
	{
		coeffFrames = 1;
	}
	else
	{
		coeffFrames = frames;
	}
	
	// set proper constant state for coefficient signals
	mA0.setConstant(paramSignalsAreConstant);
	mA1.setConstant(paramSignalsAreConstant);
	mA2.setConstant(paramSignalsAreConstant);
	mB1.setConstant(paramSignalsAreConstant);
	mB2.setConstant(paramSignalsAreConstant);
	
	float a0, a1, a2, b0, b1, b2;
	float qm1, omega, alpha, sinOmega, cosOmega;
	float highLimit = getContextSampleRate() * 0.33f;
				
	// generate coefficient signals
	// TODO SSE
	for(int n=0; n<coeffFrames; ++n)
	{
		qm1 = 1.f/(q[n] + 0.05f);
		omega = clamp(frequency[n], kLowFrequencyLimit, highLimit) * twoPiOverSr;
		sinOmega = fsin1(omega);
		cosOmega = fcos1(omega);
		alpha = sinOmega * 0.5f * qm1;
		b0 = 1.f/(1.f + alpha);
				
		switch (mode) 
		{
		default:
		case kLowpass:
			a0 = ((1.f - cosOmega) * 0.5f) * b0;
			a1 = (1.f - cosOmega);
			a2 = a0;
			b1 = (-2.f * cosOmega);
			b2 = (1.f - alpha);		
			break;
				
		case kHighpass:		
			a0 = ((1.f + cosOmega) * 0.5f);
			a1 = -(1.f + cosOmega);
			a2 = a0;
			b1 = (-2.f * cosOmega);
			b2 = (1.f - alpha);
			break;
				
		case kBandpass:
			a0 = alpha;
			a1 = 0.f;
			a2 = -alpha;
			b1 = -2.f * cosOmega;
			b2 = (1.f - alpha);
			break;
			
		case kNotch:
			a0 = 1;
			a1 = -2.f * cosOmega;
			a2 = 1;
			b1 = -2.f * cosOmega;
			b2 = (1.f - alpha);
			break;
		}
						
		mA0[n] = a0*b0;
		mA1[n] = a1*b0;
		mA2[n] = a2*b0;
		mB1[n] = b1*b0;
		mB2[n] = b2*b0;
	}
}


void MLProcBiquad::process(const int frames)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	
	// const references are necessary to only read first sample
	// in case coefficients are constant.
	const MLSignal& A0 = mA0;	
	const MLSignal& A1 = mA1;
	const MLSignal& A2 = mA2;
	const MLSignal& B1 = mB1;
	const MLSignal& B2 = mB2;
	
	calcCoeffs(frames);
	
	for (int n=0; n<frames; ++n)
	{
		float in, out;
		in = x[n];
		out = A0[n]*in + A1[n]*mX1 + A2[n]*mX2 - B1[n]*mY1 - B2[n]*mY2;
		mX2 = mX1;
		mX1 = in;
		mY2 = mY1;
		mY1 = out;
		y[n] = out;
	}
}
	   