//
//  MLT3DReceiver.h
//  MadronaLib
//
//  Created by Randy Jones on 11/7/14.
//
//

#ifndef __MLT3DReceiver__
#define __MLT3DReceiver__

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

class MLT3DHub :
	private MLOSCListener,
	private MLNetServiceHub,
	private Timer
{
public:
	static const int kFrameWidth = 4;
	static const int kFrameHeight = 16;
	static const int kFrameBufferSize = 128;

	MLT3DHub();
	~MLT3DHub();
	
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

	osc::int32 mDataRate;
	int mT3DWaitTime;
	
protected:
	// MLOSCListener
	void ProcessBundle(const osc::ReceivedBundle& b, const IpEndpointName& remoteEndpoint);
	void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint);
	
private:
	std::vector<MLT3DHub::Listener*> mpListeners;

	UdpListeningReceiveSocket* mpSocket;
	bool mSocketActive;
	pthread_t mListenerThread;

	int mUDPPortNum;
	bool mConnected;
	MLSignal mTouchFrames;
	PaUtilRingBuffer mFrameBuf;
	MLSignal mOutputFrame;
	
};


#endif /* defined(__MLT3DReceiver__) */
