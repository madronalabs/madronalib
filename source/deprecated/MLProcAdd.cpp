
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcAdd : public MLProc
{
public:
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcAdd> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcAdd> classReg("add");
	ML_UNUSED MLProcInput<MLProcAdd> inputs[] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcAdd> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

using namespace ml;

void MLProcAdd::process()
{
	// TEMP
	DSPVector va(getInput(1).getConstBuffer());
	DSPVector vb(getInput(2).getConstBuffer());
	DSPVector vsum = va + vb;
	store(vsum, getOutput().getBuffer());
	
	// TODO DSPVector i/o to Procs.
	// output() = input(1) + input(2);
}

