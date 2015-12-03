
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include <iomanip>
#include <memory>
#include <chrono>

#include "MLProc.h"
#include "MLClock.h"
#include "MLOSCSender.h"


// temporary OSC send code. 
// Procs should really not send OSC from the process thread. Instead the DSP engine can keep track of 
// some procs and send into out OSC after the main graph processing, from a different thread. 
#define SEND_OSC	1

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

#if SEND_OSC
	ml::Clock mClock;
	ml::OSCSender mOSCSender;
#endif
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
#if SEND_OSC
	mOSCSender.open(9000);
#endif
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
	const MLSignal& in = getInput(1);
	
	const int intervalSeconds = 1;
	const int intervalFrames = getContextSampleRate() * intervalSeconds;
	if (mParamsChanged) doParams();
	mTemp += frames;
	if (mTemp > intervalFrames)
	{
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
		
//		debug() << "RATE: " << getContextSampleRate() << " / " << in.getRate() << "\n";
		
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
#if SEND_OSC		
	
	// send proc name as address
	std::string address = std::string("/signal/") + getName().getString();
	
	// get Blob with signal 
	// TODO buffer
	
	// make copy of signal to hack rate change
	MLSignal xmit = in;
	if(!xmit.getRate())
	{
		xmit.setRate(getContextSampleRate());
	}	


	mOSCSender.getStream() << osc::BeginBundle(getContextTime())
	<< osc::BeginMessage( address.c_str() ) 
	<< xmit
	<< osc::EndMessage
	<< osc::EndBundle;

	mOSCSender.sendDataToSocket();	
	
//	debug() << address << " : " << in.getWidth() << " frames \n";
#endif
		
	
}




