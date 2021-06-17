// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#ifdef __WIN32__
// TODO OSC for windows

#include "catch.hpp"
#include "ip/IpEndpointName.h"
#include "ip/UdpSocket.h"
#include "madronalib.h"
#include "osc/OscOutboundPacketStream.h"

#define IP_MTU_SIZE 1536

#if 0
TEST_CASE("madronalib/core/OSC/send", "[OSC][send]")
{

	const char *hostName = "localhost";
	int port = 7000;

	IpEndpointName host( hostName, port );

	char buffer[IP_MTU_SIZE];
	osc::OutboundPacketStream p( buffer, IP_MTU_SIZE );
	UdpTransmitSocket socket( host );

	p.Clear();
	p << osc::BeginMessage( "/test1" )
					<< true << 23 << (float)3.1415 << "hello" << osc::EndMessage;
	socket.Send( p.Data(), p.Size() );
    
	//REQUIRE(a == b);

	// in progress: no tests yet
	std::cout << "OSC (TBD)\n";
	
}
#endif

#endif  // WIN32
