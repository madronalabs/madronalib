
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
	
	err resize() override;
	void clear() override;
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

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
	static const ml::Symbol timeSym("time");

	MLProc::err e = OK;
	unsigned sr = getContextSampleRate();
	mTimeInSamples = (int)(getParam(timeSym) * (float)sr);
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
	mNoiseMask = (1 << ml::bitsToContain(noisePeriodSeconds * sr)) - 1; 
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
	static const ml::Symbol gainSym("gain");

	mGain = getParam(gainSym);
	resize();
	mParamsChanged = false;
}


void MLProcAllpass::process()
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	static const MLSample noiseAmp = dBToAmp(-120.f);

	uintptr_t readIndex;
	MLSample fxn, v;

	if (mParamsChanged) calcCoeffs();
	
	// write
	for (int n=0; n<kFloatsPerDSPVector; ++n)
	{
		// zero order (integer delay)
		readIndex = mWriteIndex - mTimeInSamples;
		readIndex &= mLengthMask;
		mWriteIndex &= mLengthMask;
		
		// uninterpolated. 
		fxn = mX[readIndex];
		v = x[n] + mGain*fxn;

		// TODO remove this, again mystery denormal workaround!
		MLSample noiseHack = MLRand() * noiseAmp;
		v += noiseHack;

		y[n] = fxn - mGain*v;	
		mX[mWriteIndex++] = v;
	}
			
	// linear interp:
	//y[n] = frac*x[m+1] + (1-frac)*x[m]
	
	// allpass interp:
	// y[n] = x[m+1] + (1-frac)*x[m] - (1-frac)*y[n-1] 
	
}
