
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_OSC_LISTENER_H__
#define __ML_OSC_LISTENER_H__

#include <iostream>
#include <thread>

#include "OscTypes.h"
#include "OscException.h"
#include "OscPacketListener.h"
#include "UdpSocket.h"

class MLOSCReceiver : public osc::OscPacketListener
{
public:
	MLOSCReceiver();	// make a receiver and await further instructions - useful for member objects
	MLOSCReceiver(int port);	// make a receiver and open a port immediately
	~MLOSCReceiver();
	
	bool open(int port);
	void close();
		
	void setMessageFn(std::function<void(const osc::ReceivedMessage&)> messageFn)
	{
		mMessageFn = messageFn;
	}
	
	void setBundleStartFn(std::function<void(const osc::ReceivedBundle&)> bundleFn)
	{
		mBundleStartFn = bundleFn;
	}
	
	void setBundleEndFn(std::function<void(const osc::ReceivedBundle&)> bundleFn)
	{
		mBundleEndFn = bundleFn;
	}
	
protected:
	// implements osc::OscPacketListener
	void ProcessMessage(const osc::ReceivedMessage&, const IpEndpointName&);
	void ProcessBundle(const osc::ReceivedBundle&, const IpEndpointName&);
	
private:
	UdpListeningReceiveSocket* mpSocket;
	std::thread mListenerThread;
	std::function<void(const osc::ReceivedMessage&)> mMessageFn;
	std::function<void(const osc::ReceivedBundle&)> mBundleStartFn;
	std::function<void(const osc::ReceivedBundle&)> mBundleEndFn;

	void threadFunc();	
};

#endif // __ML_OSC_LISTENER_H__
