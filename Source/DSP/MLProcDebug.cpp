
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include <iomanip>

// ----------------------------------------------------------------
// class definition


class MLProcDebug : public MLProc
{
public:
	MLProcDebug();
	~MLProcDebug();

	void clear(){};
	void doParams();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcDebug> mInfo;
	bool mVerbose;
	int mTemp;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcDebug> classReg("debug");
	ML_UNUSED MLProcParam<MLProcDebug> params[1] = { "verbose" };
	ML_UNUSED MLProcInput<MLProcDebug> inputs[] = {"in"};
}	


// ----------------------------------------------------------------
// implementation


MLProcDebug::MLProcDebug()
{
	mTemp = 0;
	mVerbose = 0;
}

MLProcDebug::~MLProcDebug()
{
}

void MLProcDebug::doParams()
{
	mVerbose = getParam("verbose");
	mParamsChanged = false;
}

void MLProcDebug::process(const int frames)
{
	const int intervalSeconds = 4;
	const int intervalFrames = getContextSampleRate() * intervalSeconds;
	if (mParamsChanged) doParams();
	mTemp += frames;
	if (mTemp > intervalFrames)
	{
		const MLSignal& in = getInput(1);
		debug() << std::setw(6);
		debug() << std::setprecision(2);
		debug() << "sig " << getName() << " (" << static_cast<const void *>(&in) << "), n=" << frames << " = " << std::setprecision(4) << in[0] ;
		if(in.isConstant()) 
		{ 
			debug() << "(const)"; 
		}
		else
		{
			debug() << " min:" << in.getMin() << ", max:" << in.getMax();
		}
		
		debug() << "\n";
		mTemp -= intervalFrames;
		
		if (mVerbose)
		{
			debug() << frames << " frames\n";
			debug() << "[";
			debug() << std::setw(6);
			debug() << std::setprecision(2);
			for(int j=0; j<frames; ++j)
			{
				debug() << in[j] << " " ;		
				if ((j%8 == 7) && (j < frames-1)) debug() << "\n";	
			}
			debug() << "]\n\n";
		}
	}
}




