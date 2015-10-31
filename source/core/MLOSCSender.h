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
//#include "OscPacketListener.h"
#include "OscOutboundPacketStream.h"
#include "UdpSocket.h"

namespace ml {

class OSCSender 
{
public:
	OSCSender();
	OSCSender(int port);
	~OSCSender();
	
	void open(int port);
	void close();
	
	osc::OutboundPacketStream& getStream();
	void sendDataToSocket();
	
private:
	std::vector<char> mBuffer; 
	std::unique_ptr<UdpTransmitSocket> mSocket;
	std::unique_ptr<osc::OutboundPacketStream> mStream;
};

}



#endif /* defined(__Aalto__MLOscSender__) */
