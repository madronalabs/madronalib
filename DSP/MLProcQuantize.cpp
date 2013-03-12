
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition


class MLProcQuantize : public MLProc
{
public:
	MLProcQuantize();
	~MLProcQuantize();
	

	void clear(){};
	void process(const int frames);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcQuantize> mInfo;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcQuantize> classReg("quantize");
	ML_UNUSED MLProcParam<MLProcQuantize> params[1] = { "on" };	
	ML_UNUSED MLProcInput<MLProcQuantize> inputs[] = { "in" };	
	ML_UNUSED MLProcOutput<MLProcQuantize> outputs[] = { "out" };
}


// ----------------------------------------------------------------
// implementation


MLProcQuantize::MLProcQuantize()
{
	setParam("on", 1);
//	debug() << "MLProcQuantize constructor\n";
}


MLProcQuantize::~MLProcQuantize()
{
//	debug() << "MLProcQuantize destructor\n";
}

void MLProcQuantize::process(const int frames)
{
	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	int x12;
	
	if (getParam("on"))
	{
		// quantize to 12-ET
		// TODO load tuning tables
		for (int n=0; n < frames; ++n)
		{
			MLSample in = x[n];
			x12 = (float)in*12.f;
			y[n] = (float)x12 / 12.f;
		}
	}
	else
	{
		for (int n=0; n < frames; ++n)
		{
			MLSample in = x[n];
			y[n] = in;
		}
	}
}



