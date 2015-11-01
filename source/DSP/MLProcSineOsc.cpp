
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"

// ----------------------------------------------------------------
// class definition

class MLProcSineOsc : public MLProc
{
public:
	 MLProcSineOsc();
	~MLProcSineOsc();
	
	void clear();
	void doParams();
	void process(const int n);
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcSineOsc> mInfo;
    MLSineOsc mOsc;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcSineOsc> classReg("sine_osc");
	ML_UNUSED MLProcParam<MLProcSineOsc> params[1] = { "gain" };
	ML_UNUSED MLProcInput<MLProcSineOsc> inputs[] = { "frequency" }; 
	ML_UNUSED MLProcOutput<MLProcSineOsc> outputs[] = { "out" };
}			

// ----------------------------------------------------------------
// implementation

MLProcSineOsc::MLProcSineOsc()
{
	clear();
}

MLProcSineOsc::~MLProcSineOsc()
{
}

void MLProcSineOsc::clear()
{
    mOsc.clear();
}

void MLProcSineOsc::doParams()
{
    mOsc.setSampleRate(getContextSampleRate());
    mParamsChanged = false;
}

void MLProcSineOsc::process(const int samples)
{
	if (mParamsChanged) doParams();
    const MLSignal& freq = getInput(1);
    MLSignal& out = getOutput(1);
	
	for (int n=0; n<samples; ++n)
	{
        mOsc.setFrequency(freq[n]);
        out[n] = mOsc.processSample();
	}
}
