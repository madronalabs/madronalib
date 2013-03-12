
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcFade : public MLProc
{
public:
	 MLProcFade();
	~MLProcFade();	
	void process(const int n);		
	void clear();
	MLProcInfoBase& procInfo() { return mInfo; }
	
private:
	MLProcInfo<MLProcFade> mInfo;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcFade> classReg("fade");
	// no parameters.  ML_UNUSED MLProcParam<MLProcFade> params[1] = { "" };
	ML_UNUSED MLProcInput<MLProcFade> inputs[] = {"in1", "in2", "mix"}; 
	ML_UNUSED MLProcOutput<MLProcFade> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

MLProcFade::MLProcFade()
{
}

MLProcFade::~MLProcFade()
{
}

void MLProcFade::clear()
{
}

void MLProcFade::process(const int frames)
{	
	const MLSignal& in1 = getInput(1);
	const MLSignal& in2 = getInput(2);
	const MLSignal& mix = getInput(3);
	MLSignal& out = getOutput();
	
	for (int n=0; n<frames; ++n)
	{
		MLSample a = in1[n];
		MLSample b = in2[n];
		out[n] = a + (b - a)*mix[n];
	}
	
#ifdef DEBUG
	// test output!
	int count = 0;
	for (int n=0; n<frames; ++n)
	{
		MLSample k = out[n];
		if (k != k)
		{
			count++;
		}
	}
	if (count > 0)
	{
		debug() << "MLProcFade " << getName() << ": " << count << " NaN samples!\n" ;
		float a = in1[0]; float b = in2[0]; float c = mix[0];
		if (a != a) debug() << "    in1 NaN!\n";
		if (b != b) debug() << "    in2 NaN!\n";
		if (c != c) debug() << "    mix NaN!\n";
	}
#endif

}
