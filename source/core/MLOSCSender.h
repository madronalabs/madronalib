//
//  MLOscSender.h
//  Aalto
//
//  Created by Randy Jones on 10/30/15.
//
//

#ifndef __Aalto__MLOscSender__
#define __Aalto__MLOscSender__

#include <stdexcept>
#include <iostream>
#include <thread>
#include <vector>

#include "OscTypes.h"
#include "OscException.h"
#include "OscOutboundPacketStream.h"
#include "UdpSocket.h"

#include "MLSignal.h"

namespace ml {
	
class OSCSender 
{
public:
	OSCSender();
	OSCSender(int port);
	~OSCSender();
	
	void open(int port);
	void close();
	
private:
	typedef osc::OutboundPacketStream PacketStream;
	 
public:	
	// TODO better interface. Would like to do something like
	// OSCSender s;
	// float f, g, h;
	// s.sendNow(bundle(message("/a", f), message("/b", g, h, 23)));
	// This requires a bunch of template cruft I don't have time for right now
	PacketStream& getStream();
	void sendDataToSocket();
	
private:
	std::vector<char> mBuffer; 
	std::unique_ptr<UdpTransmitSocket> mSocket;
	std::unique_ptr<PacketStream> mStream;
};


}	// namespace ml

osc::OutboundPacketStream& operator<< (osc::OutboundPacketStream& stream, const MLSignal& sig);


#endif /* defined(__Aalto__MLOscSender__) */
