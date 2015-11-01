
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include <iomanip>
#include <memory>
#include <chrono>

#include "MLProc.h"
#include "MLClock.h"
#include "MLOSCSender.h"

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
	const int intervalSeconds = 1;
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
		
#if SEND_OSC		
		uint64_t ntpTime = mClock.now();
		
		// send proc name as address
		std::string address = std::string("/signal/") + getName().getString();
		
		// get Blob with signal 
		// TODO buffer
		
		// TODO xmit signal dims
		
		MLSignal in2({1, 3, 5, 7, 9});
		MLSignal in3(3, 4);
		for(int i=0; i<3; i++)
		{
			for(int j=0; j<4; j++)
			{
				in3(i, j) = MLRand();
			}
		}
		
//		osc::Blob b(in2.getBuffer(), in2.getWidth()*sizeof(float));
	
		mOSCSender.getStream() << osc::BeginBundle(ntpTime)
		<< osc::BeginMessage( address.c_str() ) 

		<< in3
		<< osc::EndMessage
		<< osc::EndBundle;

		mOSCSender.sendDataToSocket();	
#endif
		
	}
}




