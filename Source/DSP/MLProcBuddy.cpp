
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// MLProcBuddy: a proc for synching two signals. The way the DSP graph is designed,
// this kind of object should not be needed. But here we are.
// motivation is a quick fix for xy displays-- signals are coming out of the
// graph not quite synched right sometimes.
// TODO fix that by changing published signal mechanism to more of a push model
// as discussed elsewhere, then get rid of this object. #include "MLProcBuddy.h"


#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcBuddy : public MLProc
{
public:
	MLProcBuddy();
	~MLProcBuddy();
	
    
	void clear(){};
	void process(const int frames);
	MLProcInfoBase& procInfo() { return mInfo; }
    
private:
	MLProcInfo<MLProcBuddy> mInfo;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcBuddy> classReg("buddy");
	// no parameters
	ML_UNUSED MLProcInput<MLProcBuddy> inputs[2] = {"in1", "in2"};
	ML_UNUSED MLProcOutput<MLProcBuddy> outputs[2] = {"out1", "out2"};
}


// ----------------------------------------------------------------
// implementation


MLProcBuddy::MLProcBuddy()
{
    //	debug() << "MLProcBuddy constructor\n";
}


MLProcBuddy::~MLProcBuddy()
{
    //	debug() << "MLProcBuddy destructor\n";
}

void MLProcBuddy::process(const int frames)
{
    const MLSignal& x1 = getInput(1);
    const MLSignal& x2 = getInput(2);
	MLSignal& y1 = getOutput(1);
	MLSignal& y2 = getOutput(2);
    
    y1 = x1;
    y2 = x2;

}



