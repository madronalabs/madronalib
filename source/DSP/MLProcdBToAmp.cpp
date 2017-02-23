
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
using namespace ml;

// ----------------------------------------------------------------
// class definition

class MLProcdBToAmp : public MLProc
{
public:
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcdBToAmp> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcdBToAmp> classReg("db_to_amp");
	ML_UNUSED MLProcParam<MLProcdBToAmp> params[] = { };
	ML_UNUSED MLProcInput<MLProcdBToAmp> inputs[] = { "in" };
	ML_UNUSED MLProcOutput<MLProcdBToAmp> outputs[] = {"out"};
}	

// ----------------------------------------------------------------
// implementation

void MLProcdBToAmp::process()
{
	const DSPVector* pvin = reinterpret_cast<const DSPVector*>(getInput(1).getConstBuffer());
	DSPVector* pvout = reinterpret_cast<DSPVector*>(getOutput().getBuffer());
	(*pvout) = pow(DSPVector(10.f), (*pvin)/DSPVector(20.f));
}

