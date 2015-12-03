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

	// OSCSender
	
	OSCSender::OSCSender()
	{
		mBuffer.resize(kBufSize);
		mStream = std::unique_ptr<OSCSender::PacketStream>(new OSCSender::PacketStream(mBuffer.data(), kBufSize));
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
	
	OSCSender::PacketStream& OSCSender::getStream()
	{
		mStream->Clear();
		return *mStream;
	}
	
	void OSCSender::sendDataToSocket()
	{
		mSocket->Send(mStream->Data(), mStream->Size());
	}
}	// namespace ml 


// stream operators for madronalib types

osc::OutboundPacketStream& operator<< (osc::OutboundPacketStream& stream, const MLSignal& sig)
{
	stream << sig.getWidth() << sig.getHeight() << sig.getDepth() << static_cast<int>(sig.getRate());
	stream << osc::Blob(sig.getConstBuffer(), sig.getSize()*sizeof(float));
	return stream;
}

