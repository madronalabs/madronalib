
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"

// ----------------------------------------------------------------
// class definition

class MLProcSpectralPeak : public MLProc
{
public:
	MLProcSpectralPeak();
	~MLProcSpectralPeak();
	
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
private:
	MLProcInfo<MLProcSpectralPeak> mInfo;
	void calcCoeffs(void);
	
	float mThresh;
	
	MLBiquad mEnvFilter;
	MLSineOsc mTestOsc;
};

// ----------------------------------------------------------------
// registry section

namespace{
	
	MLProcRegistryEntry<MLProcSpectralPeak> classReg("spectral_peak");
	//ML_UNUSED MLProcParam<MLProcSpectralPeak> params[] = {"thresh"};
	ML_UNUSED MLProcInput<MLProcSpectralPeak> inputs[] = {"in"};
	ML_UNUSED MLProcOutput<MLProcSpectralPeak> outputs[] = {"peak"};
	
}	// namespace

// ----------------------------------------------------------------
// implementation

MLProcSpectralPeak::MLProcSpectralPeak()
{
}

MLProcSpectralPeak::~MLProcSpectralPeak()
{
}

void MLProcSpectralPeak::clear()
{
	mThresh = 0.001f;
}

// generate envelope output based on gate and control signal inputs.
void MLProcSpectralPeak::process(const int samples)
{	
	//float invSr = getContextInvSampleRate();
//	const MLSignal& in = getInput(1);
	MLSignal& peak = getOutput(1);	
	
	if (mParamsChanged)
	{
		static MLSymbol threshSym("thresh");
		mThresh = getParam(threshSym);
		int sr = getContextSampleRate();
		mEnvFilter.setSampleRate(sr);
		mEnvFilter.setLopass(20.f, 0.707f);
		
		mTestOsc.setSampleRate(sr);
		mTestOsc.setFrequency(1.23f);
		
		mParamsChanged = false;
	}
	
	for (int n=0; n<samples; ++n)
	{
		// test
		peak[n] = MLRand();//mTestOsc.processSample();
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



