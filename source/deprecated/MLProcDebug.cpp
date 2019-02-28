
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include <iomanip>
#include <memory>
#include <chrono>

#include "MLProc.h"
#include "MLClock.h"

#ifdef _WINDOWS
#define UNICODE
#include <Windows.h>
#endif

// temporary OSC send code. 
// Procs should really not send OSC from the process thread. Instead the DSP engine can keep track of 
// some procs and send into out OSC after the main graph processing, from a different thread. 
#define SEND_OSC	DEBUG&&__APPLE__

#ifdef SEND_OSC
#include "MLOSCSender.h"
#endif

// ----------------------------------------------------------------
// class definition

class MLProcDebug : public MLProc
{
public:
	MLProcDebug();
	~MLProcDebug();

	void doParams();
	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

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
	static const ml::Symbol vSym("verbose");

	mVerbose = getParam(vSym);
	mParamsChanged = false;
}

void MLProcDebug::process()
{
	const MLSignal& in = getInput(1);
	
	const int intervalSeconds = 1;
	const int intervalFrames = getContextSampleRate() * intervalSeconds;
	if (mParamsChanged) doParams();
	mTemp += kFloatsPerDSPVector;
	if (mTemp > intervalFrames)
	{
		//debug() << std::setw(6);
		//debug() << std::setprecision(2);
		//debug() << "sig " << getName() << " (" << static_cast<const void *>(&in) << "), n=" << kFloatsPerDSPVector << " = " << std::setprecision(4) << in[0] ;
		//debug() << " min:" << in.getMin() << ", max:" << in.getMax();
		//debug() << "\n";
//		//debug() << "RATE: " << getContextSampleRate() << " / " << in.getRate() << "\n";
		
		mTemp -= intervalFrames;
		
		if (mVerbose)
		{
			//debug() << kFloatsPerDSPVector << " frames\n";
			//debug() << "[";
			//debug() << std::setw(6);
			//debug() << std::setprecision(2);
			for(int j=0; j<kFloatsPerDSPVector; ++j)
			{
				//debug() << in[j] << " " ;		
				if ((j%8 == 7) && (j < kFloatsPerDSPVector-1)) //debug() << "\n";	
			}
			//debug() << "]\n\n";
		}

#ifdef _WINDOWS

		// TODO dodgy temporary code, fix
		const int kWideBufSize = 16384;
		static wchar_t wideBuf[kWideBufSize];
		const int kMaxChars = 256;
		static char charBuf[kMaxChars];

		ml::TextFragment t(getName().getTextFragment());
		snprintf(charBuf, kMaxChars, "sig %s: min %f, max %f\n\0", t.getText(), in.getMin(), in.getMax());

		MultiByteToWideChar(0, 0, charBuf, -1, wideBuf, kWideBufSize);
		OutputDebugString(wideBuf);

#endif


	}
#if SEND_OSC		
	
	// send proc name as address
	std::string address = std::string("/signal/") + getName().toString();
	
	// get Blob with signal 
	// TODO buffer
	
	// make copy of signal to hack rate change
	MLSignal xmit;
	xmit = in;
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
	
//	//debug() << address << " : " << in.getWidth() << " frames \n";
#endif
		
	
}




