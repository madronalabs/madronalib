
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcAllpass : public MLProc
{
public:
	 MLProcAllpass();
	~MLProcAllpass();
	
	err resize();
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcAllpass> mInfo;
	void calcCoeffs(void);
	
	MLSignal mX;	// direct form 2, uses only one delay
	uintptr_t mWriteIndex;
	uintptr_t mLengthMask;
	uintptr_t mNoiseIndex;
	uintptr_t mNoiseMask;
	int mTimeInSamples;
	float mGain;
	float mNoiseGain;
	float mNoisePeriodSeconds;
	float mOneOverNoiseDomain;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcAllpass> classReg("allpass");
	ML_UNUSED MLProcParam<MLProcAllpass> params[2] = { "time", "gain" };
	ML_UNUSED MLProcInput<MLProcAllpass> inputs[] = {"in"}; 
	ML_UNUSED MLProcOutput<MLProcAllpass> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

MLProcAllpass::MLProcAllpass()
{
	setParam("time", 0.25f);
	setParam("gain", 0.5f);
	mGain = 0.5f;
	mNoiseGain = 0.f;
	mNoiseIndex = 0;
}

MLProcAllpass::~MLProcAllpass()
{
}

MLProc::err MLProcAllpass::resize() 
{	
	MLProc::err e = OK;
	unsigned sr = getContextSampleRate();
	mTimeInSamples = (int)(getParam("time") * (float)sr);
// debug() << "allpass::resize: " << 	mTimeInSamples << " samples.\n";
	unsigned bufferLength = mTimeInSamples;
	
	if (!mX.setDims(bufferLength + 1))
	{
		e = memErr;
		mLengthMask = 0;
	}
	else
	{
		mLengthMask = (1 << mX.getWidthBits()) - 1;
	}
	
	unsigned noisePeriodSeconds = 30;
	mNoiseMask = (1 << bitsToContain(noisePeriodSeconds * sr)) - 1; 
	mOneOverNoiseDomain = 1.f / (float)(mNoiseMask + 1);
	mNoisePeriodSeconds = (float)(mNoiseMask + 1) / (float)sr;

	return e;
}

void MLProcAllpass::clear() 
{	
	mX.clear();
	mWriteIndex = 0;
}


void MLProcAllpass::calcCoeffs() 
{
	mGain = getParam("gain");
	
#if DEMO
	if (mGain == 0.6255f)
	{
		mNoiseGain = 0.5;		
	}
#endif	
	resize();

	mParamsChanged = false;
}


void MLProcAllpass::process(const int frames)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	static const MLSample noiseAmp = dBToAmp(-120.f);

	uintptr_t readIndex;
	MLSample fxn, v;
#if DEMO
	unsigned sr = getContextSampleRate();
	float invSr = getContextInvSampleRate();
	MLSample noise = 0.f;
	float noiseX = 0.f;
	int noiseOffset;	
#endif

	if (mParamsChanged) calcCoeffs();
	
	// write
	for (int n=0; n<frames; ++n)
	{
		// zero order (integer delay)
		readIndex = mWriteIndex - mTimeInSamples;
		readIndex &= mLengthMask;
		mWriteIndex &= mLengthMask;
	
#if DEMO
		mNoiseIndex &= mNoiseMask;
		float p25 = 0.25f;
		
		// scale [0-sr] -> [0, 1]
		// window fn is in seconds from -sqrt8 to sqrt8.
		noiseOffset = ((int)mNoiseIndex) - (sr << 4);
		noiseX = (float)(noiseOffset) * invSr;
	
		float w, xc, xc2, xc4;
		const float sqrt8 = 2.82842712475f;

		xc = clamp(noiseX, -sqrt8, sqrt8); 
		xc2 = xc*xc;
		xc4 = xc2 * xc2;
		w = (1.f - xc2*p25 + xc4*0.015625f) * p25;
		
		noise = MLRand() * w;		
		mNoiseIndex++;
#endif		
		
		// uninterpolated. 
		fxn = mX[readIndex];
		v = x[n] + mGain*fxn;

		// TODO remove this, again mystery denormal workaround!
		MLSample noiseHack = MLRand() * noiseAmp;
		v += noiseHack;

#if DEMO
		v += mNoiseGain*noise;
#endif
		y[n] = fxn - mGain*v;	
		mX[mWriteIndex++] = v;
	}
			
	// linear interp:
	//y[n] = frac*x[m+1] + (1-frac)*x[m]
	
	// allpass interp:
	// y[n] = x[m+1] + (1-frac)*x[m] - (1-frac)*y[n-1] 
	
}
