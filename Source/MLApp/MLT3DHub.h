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
#include "MLProjectInfo.h"
#include "MLOSCListener.h"
#include "MLNetServiceHub.h"
#include "MLDebug.h"
#include "MLSignal.h"
#include "MLSymbol.h"

#include "pa_ringbuffer.h"
#include "pthread.h"
#include <stdexcept>

#include "JuceHeader.h"

class MLT3DHub :
	public MLNetServiceHub,
	private MLOSCListener,
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
		virtual void handleHubNotification(MLSymbol action, const float val) = 0;
	};
	
	void addListener(Listener* pL);
	void removeListener(Listener* pL);
	void notifyListeners(MLSymbol action, const float val);
	void timerCallback();
	void setOSCPortOffset(int offset);
	
	osc::int32 mDataRate;
	int mT3DWaitTime;
	
protected:
	// MLOSCListener
	void ProcessBundle(const osc::ReceivedBundle& b, const IpEndpointName& remoteEndpoint);
	void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint);
	
private:
	void connect();
	void disconnect();

	std::vector<MLT3DHub::Listener*> mpListeners;

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
