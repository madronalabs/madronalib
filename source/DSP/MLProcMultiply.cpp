
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
	const DSPVector* pvm1 = reinterpret_cast<const DSPVector*>(getInput(1).getConstBuffer());
	const DSPVector* pvm2 = reinterpret_cast<const DSPVector*>(getInput(2).getConstBuffer());
	DSPVector* pvout = reinterpret_cast<DSPVector*>(getOutput().getBuffer());
	(*pvout) = (*pvm1)*(*pvm2);
}

