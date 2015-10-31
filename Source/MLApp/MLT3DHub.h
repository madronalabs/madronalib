//
//  MLT3DHub.h
//  MadronaLib
//
//  Created by Randy Jones on 11/7/14.
//
//

#ifndef __MLT3DHUB__
#define __MLT3DHUB__

#if defined (__APPLE__)

#include <iostream>

#include "MLDSP.h"
#include "MLPlatform.h"
#include "MLOSCReceiver.h"
#include "MLNetServiceHub.h"
#include "MLDebug.h"
#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLProperty.h"

#include "pa_ringbuffer.h"
#include "pthread.h"
#include <stdexcept>

#include "JuceHeader.h"

class MLT3DHub :
	public MLNetServiceHub,
	private juce::Timer
{
public:
	static const int kFrameWidth = 4;
	static const int kFrameHeight = 16;
	static const int kFrameBufferSize = 128;
	static const int kDefaultUDPPort = 3123;

	MLT3DHub();
	~MLT3DHub();
	
	void setEnabled(int e);
	int getPortOffset() { return mUDPPortOffset; }
	void setPortOffset(int offset);
	
	void didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing);

	PaUtilRingBuffer* getFrameBuffer() { return &mFrameBuf; }
	
    class Listener
	{
		friend class MLT3DHub;
	public:
		Listener() {}
		virtual ~Listener() {}
	protected:
		virtual void handleHubNotification(MLSymbol action, const MLProperty val) = 0;
	};
	
	void addListener(Listener* pL);
	void removeListener(Listener* pL);
	void notifyListeners(MLSymbol action, const MLProperty val);
	void timerCallback();
	void setOSCPortOffset(int offset);
	
	void setShortName(const std::string& n) { mShortName = n; }
	
	osc::int32 mDataRate;
	int mT3DWaitTime;
	
private:
	void connect();
	void disconnect();
	void handleMessage(const osc::ReceivedMessage& m);
	void endBundle(const osc::ReceivedBundle& b);
	
	MLOSCReceiver mOSCReceiver;

	// listeners to this object
	std::vector<MLT3DHub::Listener*> mpListeners;
	
	std::string mShortName; // will append a port # to this to create full name of MLNetServiceHub

	int mEnabled;
	int mUDPPortOffset;
	
	// true if we have actually received OSC on our connected port.
	bool mReceivingT3d;
	
	bool mConnected;
	bool mShouldConnect;
	bool mShouldDisconnect;
	MLSignal mTouchFrames;
	PaUtilRingBuffer mFrameBuf;
	MLSignal mOutputFrame;
	
};

#endif // APPLE

#endif /* defined(__MLT3DHub__) */
