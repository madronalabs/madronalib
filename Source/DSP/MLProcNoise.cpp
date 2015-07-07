
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include <iostream>
#include <string>
#include <math.h>
#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcNoise : public MLProc
{
public:
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcNoise> mInfo;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcNoise> classReg("noise");
	ML_UNUSED MLProcParam<MLProcNoise> params[1] = { "gain" };
	//ML_UNUSED MLProcInput<MLProcNoise> inputs[] = {}; 
	ML_UNUSED MLProcOutput<MLProcNoise> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

void MLProcNoise::process(const int samples)
{	
	static MLSymbol gainSym("gain");
	MLSignal& y = getOutput();
	y.setConstant(false); // ?
	MLSample gain = getParam(gainSym);
	
	for (int n=0; n<samples; ++n)
	{
		y[n] = MLRand() * gain;
	}
}


