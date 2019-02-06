
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcMultiplyAdd : public MLProc
{
public:
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }
	
private:
	MLProcInfo<MLProcMultiplyAdd> mInfo;
};

// ----------------------------------------------------------------
// registry

namespace
{
	MLProcRegistryEntry<MLProcMultiplyAdd> classReg("multiply_add");
	ML_UNUSED MLProcInput<MLProcMultiplyAdd> inputs[] = {"m1", "m2", "a1"};
	ML_UNUSED MLProcOutput<MLProcMultiplyAdd> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

using namespace ml;

void MLProcMultiplyAdd::process()
{
	const DSPVector va(getInput(1).getConstBuffer());
	const DSPVector vb(getInput(2).getConstBuffer());
	const DSPVector vc(getInput(3).getConstBuffer());
	DSPVector vp = va * vb + vc;
	store(vp, getOutput().getBuffer());
}

