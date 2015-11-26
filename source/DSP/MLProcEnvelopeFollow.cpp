
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
	
	float mThreshUpper, mThreshLower;

	MLAsymmetricOnepole mTrigFilter;
	MLAsymmetricOnepole mEnvFilter;
	bool mTrig;
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
	setParam("thresh", -12);
	clear();
}

MLProcEnvelopeFollow::~MLProcEnvelopeFollow()
{
}

void MLProcEnvelopeFollow::clear()
{
	mTrig = false;
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
		const float kLogHysteresis = 18.f;
		float logThresh = getParam("thresh");
		mThreshUpper = dBToAmp(logThresh);
		mThreshLower = dBToAmp(logThresh - kLogHysteresis);
		int sr = getContextSampleRate();
		mTrigFilter.setSampleRate(sr);
		mTrigFilter.setAttackAndReleaseTimes(0.001f, 0.005f);
		mEnvFilter.setSampleRate(sr);
		mEnvFilter.setAttackAndReleaseTimes(0.01f, 0.5f);
		mParamsChanged = false;
	}
	
	for (int n=0; n<samples; ++n)
	{
		float y = fabs(in[n]);
		float yf = mTrigFilter.processSample(y);
		
		if(mTrig)
		{
			if(yf < mThreshLower)
			{
				mTrig = false;
			}
		}
		else
		{
			if(yf > mThreshUpper)
			{
				mTrig = true;
			}
		}
		trig[n] = (mTrig) ? 1.0f : 0.f;
		
		// test
		// use asym filter, vactrol
		env[n] = (mEnvFilter.processSample(y));
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



