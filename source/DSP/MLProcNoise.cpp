
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include <iostream>
#include <string>
#include <math.h>
#include "MLProc.h"

using namespace ml;

// ----------------------------------------------------------------
// class definition

class MLProcNoise : public MLProc
{
public:
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	RandomSource mRandomSource;
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

static ml::Symbol gainSym("gain");

// ----------------------------------------------------------------
// implementation

void MLProcNoise::process()
{	
	store(mRandomSource(), getOutput().getBuffer());
}


