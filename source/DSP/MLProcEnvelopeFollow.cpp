
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"

// ----------------------------------------------------------------
// class definition

class MLProcEnvelopeFollow : public MLProc
{
public:
	MLProcEnvelopeFollow();
	~MLProcEnvelopeFollow();
	
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
private:
	MLProcInfo<MLProcEnvelopeFollow> mInfo;
	void calcCoeffs(void);
	
	float mThresh;

	MLBiquad mEnvFilter;
};

// ----------------------------------------------------------------
// registry section

namespace{
	
	MLProcRegistryEntry<MLProcEnvelopeFollow> classReg("envelope_follow");
	ML_UNUSED MLProcParam<MLProcEnvelopeFollow> params[] = {"thresh"};
	ML_UNUSED MLProcInput<MLProcEnvelopeFollow> inputs[] = {"in"};
	ML_UNUSED MLProcOutput<MLProcEnvelopeFollow> outputs[] = {"trig", "env"};
	
}	// namespace

// ----------------------------------------------------------------
// implementation

MLProcEnvelopeFollow::MLProcEnvelopeFollow()
{
}

MLProcEnvelopeFollow::~MLProcEnvelopeFollow()
{
}

void MLProcEnvelopeFollow::clear()
{
	mThresh = 0.001f;
}

// generate envelope output based on gate and control signal inputs.
void MLProcEnvelopeFollow::process(const int samples)
{	
	//float invSr = getContextInvSampleRate();
	const MLSignal& in = getInput(1);
	MLSignal& trig = getOutput(1);	
	MLSignal& env = getOutput(2);
	
	if (mParamsChanged)
	{
		static MLSymbol threshSym("thresh");
		mThresh = getParam(threshSym);
		int sr = getContextSampleRate();
		mEnvFilter.setSampleRate(sr);
		mEnvFilter.setLopass(20.f, 0.707f);
		mParamsChanged = false;
	}
	
	for (int n=0; n<samples; ++n)
	{
		// test
		float y = in[n];
		trig[n] = static_cast<float>(y > mThresh);
		
		// test
		// use asym filter, vactrol
		env[n] = sqrtf(mEnvFilter.processSample(y*y));
	}
	
	/*
	 int sr = getContextSampleRate();
	 mT += samples;
	 if (mT > sr)
	 {
	 //	debug() << getName() << " state: " << mState << " env: " << mEnv << "\n";
		debug() << "trig: " << trigSelect << "\n";
		mT -= sr;
	 }
	 */
}

// TODO envelope sometimes sticks on for very fast gate transients.  Rewrite this thing!



