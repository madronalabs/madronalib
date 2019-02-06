
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcMultiply : public MLProc
{
public:
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }
	
private:
	MLProcInfo<MLProcMultiply> mInfo;
};

// ----------------------------------------------------------------
// registry

namespace
{
	MLProcRegistryEntry<MLProcMultiply> classReg("multiply");
	ML_UNUSED MLProcInput<MLProcMultiply> inputs[] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcMultiply> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

using namespace ml;

void MLProcMultiply::process()
{
	const DSPVector va(getInput(1).getConstBuffer());
	const DSPVector vb(getInput(2).getConstBuffer());
	DSPVector vp = va * vb;
	store(vp, getOutput().getBuffer());
}

