
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLOSCReceiver.h"

MLOSCReceiver::MLOSCReceiver() :
mpSocket(0)
{
}

MLOSCReceiver::MLOSCReceiver(int port) :
mpSocket(0)
{
	open(port);
}

MLOSCReceiver::~MLOSCReceiver()
{
	close();
}
				   
bool MLOSCReceiver::open(int port)
{
	bool ret = false;
	if(mpSocket)
	{
		// disconnect existing socket
		close();
	}

	try
	{
		// std::cout << "MLOSCReceiver: trying to receive on port " << port << "...\n";
		// the new UdpListeningReceiveSocket has this as its listener, so it will call our ProcessMessage()
		// and ProcessBundle() methods to handle incoming messages.
		mpSocket = new UdpListeningReceiveSocket(
			IpEndpointName( IpEndpointName::ANY_ADDRESS, port), 
			this);
	}
	catch( osc::Exception& e )
	{
		mpSocket = 0;
		std::cout << "MLOSCReceiver::open: couldn't bind to port " << port << ".\n";
		std::cout << "error: " << e.what() << "\n";
	}
	catch(...)
	{
		mpSocket = 0;
		std::cout << "MLOSCReceiver::open: couldn't bind to port " << port << ".\n";
		std::cout << "Unknown error.\n";
	}

	if(mpSocket)
	{
		// std::cout << "MLOSCReceiver::open: receiving on port " << port << ".\n";		
		// make thread and send it on its way. It will terminate when mpSocket->Break() is called.
		// passing "this" allows the thread to call our member function. Note that this object must be careful
		// to outlive the thread.
		std::thread t(&MLOSCReceiver::threadFunc, this);
		t.detach();
		ret = true;
	}
	return ret;
}

void MLOSCReceiver::close()
{
	if(mpSocket)
	{
		mpSocket->Break();
		delete mpSocket;
		mpSocket = 0;
	}
}

void MLOSCReceiver::ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName&)
{
	if(mMessageFn) mMessageFn(m);
}

void MLOSCReceiver::ProcessBundle(const osc::ReceivedBundle& b, const IpEndpointName& remoteEndpoint)
{
	if(mBundleStartFn) mBundleStartFn(b);
	
	for( osc::ReceivedBundle::const_iterator i = b.ElementsBegin(); i != b.ElementsEnd(); ++i )
	{
		if( i->IsBundle() )
			ProcessBundle( osc::ReceivedBundle(*i), remoteEndpoint );
		else
			ProcessMessage( osc::ReceivedMessage(*i), remoteEndpoint );
	}

	if(mBundleEndFn) mBundleEndFn(b);
}

void MLOSCReceiver::threadFunc(void)
{
	try
	{
		mpSocket->Run();
	}
	catch( osc::Exception& e )
	{
		std::cout << "MLOSCReceiver caught osc exception: " << e.what() << "\n";
	}
	catch( std::runtime_error& e )
	{
		std::cout << "MLOSCReceiver caught runtime_error exception: " << e.what() << "\n";
	}
}

