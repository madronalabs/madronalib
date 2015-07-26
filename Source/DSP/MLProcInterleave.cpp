
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// MLProcInterleave: a proc for synching two signals. The way the DSP graph is designed,
// this kind of object should not be needed. But here we are.
// motivation is a quick fix for xy displays-- signals are coming out of the
// graph not quite synched right sometimes.
// TODO fix that by changing published signal mechanism to more of a push model
// as discussed elsewhere, then get rid of this object. 


#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcInterleave : public MLProc
{
public:
	void process(const int frames);
	MLProcInfoBase& procInfo() { return mInfo; }
    
private:
	MLProcInfo<MLProcInterleave> mInfo;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcInterleave> classReg("interleave");
	// no parameters
	ML_UNUSED MLProcInput<MLProcInterleave> inputs[2] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcInterleave> outputs[1] = {"out"};
}


// ----------------------------------------------------------------
// implementation

void MLProcInterleave::process(const int frames)
{
    const MLSignal& x1 = getInput(1);
    const MLSignal& x2 = getInput(2);
	MLSignal& y = getOutput();
    
    for(int i=0; i<frames; i+= 2)
    {
        y[i] = x1[i];
        y[i + 1] = x2[i];
    }
}



