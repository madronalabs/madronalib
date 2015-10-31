//
//  MLOscSender.cpp
//  Aalto
//
//  Created by Randy Jones on 10/30/15.
//
//

#include "MLOscSender.h"

const int kBufSize = 4096;

namespace ml {

	OSCSender::OSCSender()
	{
		mBuffer.resize(kBufSize);
		mStream = std::unique_ptr<osc::OutboundPacketStream>(new osc::OutboundPacketStream ( mBuffer.data(), kBufSize ));
	}
	
	OSCSender::OSCSender(int port)
	{
		open(port);
	}

	OSCSender::~OSCSender()
	{
		
	}

	void OSCSender::open(int port)
	{
		mSocket = std::unique_ptr<UdpTransmitSocket>(new UdpTransmitSocket( IpEndpointName("localhost", port)));
	}
	
	void OSCSender::close()
	{
		
	}
	
	// TEMP
	osc::OutboundPacketStream& OSCSender::getStream()
	{
		mStream->Clear();
		return *mStream;
	}
	
	// TEMP
	void OSCSender::sendDataToSocket()
	{
		mSocket->Send(mStream->Data(), mStream->Size());
	}
}
