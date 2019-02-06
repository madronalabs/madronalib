
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/


#ifndef __ML_OSC_LISTENER_H__
#define __ML_OSC_LISTENER_H__

#include "MLPlatform.h"

#if !ML_WINDOWS

#include "OscTypes.h"
#include "OscException.h"
#include "OscPacketListener.h"
#include "UdpSocket.h"

#include <stdexcept>
#include <iostream>
#include "pthread.h"
	
// start and run a listener thread, not returning until the thread is done.
// used as argument to pthread_create().
void * MLOSCListenerStartThread(void *arg);

class MLOSCListener : public osc::OscPacketListener
{
friend void * MLOSCListenerStartThread(void *arg);

public:
	MLOSCListener();
	virtual ~MLOSCListener();
	
	// listen to the given port or, if port = 0, shut down listening gear. Return success.
	int listenToOSC(int port);
	int getPort() { return mPort; }
		
protected:
    virtual void ProcessMessage(const osc::ReceivedMessage&, const IpEndpointName& ) = 0;
	virtual void ProcessBundle(const osc::ReceivedBundle& , const IpEndpointName& ) = 0;

	bool mListening;

private:
	UdpListeningReceiveSocket* mpSocket;
	int mPort;
	pthread_t mListenerThread;
};

#endif // __APPLE__

#endif // __ML_OSC_LISTENER_H__
