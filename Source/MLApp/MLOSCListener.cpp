
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPlatform.h"

#if ML_WINDOWS
	// TODO
#else
	
#include "MLOSCListener.h"

MLOSCListener::MLOSCListener() :
	mListening(false),
	mpSocket(0),
	mPort(0)
{
}

MLOSCListener::~MLOSCListener()
{
	listenToOSC(0);
}

void * MLOSCListenerStartThread(void *arg)
{
	MLOSCListener* pL = static_cast<MLOSCListener*>(arg);
	UdpListeningReceiveSocket* pSocket = pL->mpSocket;

	try
	{
		pSocket->Run();
	}
	catch( osc::Exception& e )
	{
		std::cout << "MLOSCListener caught osc exception: " << e.what() << "\n";
	}
	catch( std::runtime_error& e )
	{
		std::cout << "MLOSCListener caught runtime_error exception: " << e.what() << "\n";
	}
	
	// std::cout << "MLOSCListener: listener thread on port " << port << " terminated.\n";
	return 0;
}

int MLOSCListener::listenToOSC(int port)
{
	int ret = false;
	if(port)
	{
		if(mpSocket)
		{
			listenToOSC(0);
		}
		try
		{
			std::cout << "MLOSCListener: trying listen on port " << port << "...\n";
			mpSocket = new UdpListeningReceiveSocket(
				IpEndpointName( IpEndpointName::ANY_ADDRESS, port), 
				this);
		}
		catch( osc::Exception& e )
		{
			mpSocket = 0;
			std::cout << "MLOSCListener::listenToOSC: couldn't bind to port " << port << ".\n";
			std::cout << "error: " << e.what() << "\n";
		}
		catch(...)
		{
			mpSocket = 0;
			std::cout << "MLOSCListener::listenToOSC: couldn't bind to port " << port << ".\n";
			std::cout << "Unknown error.\n";
		}
		
		if(mpSocket)
		{
			std::cout << "MLOSCListener::listenToOSC: created receive socket on port " << port << ".\n";
			mPort = port;
			
			int err;
			pthread_attr_t attr;
			
			// std::cout << "initializing pthread attributes...\n";
			err = pthread_attr_init(&attr);

			if(!err)
			{
				// std::cout << "creating listener thread...\n";
				err = pthread_create(&mListenerThread, &attr, &MLOSCListenerStartThread, (void*)this);
				
				if(!err)
				{
					ret = true;
					mListening = true;
				}
			}
		}
	}
	else
	{
		if(mpSocket)
		{
			std::cout << "MLOSCListener: disconnecting.\n";
			mpSocket->Break();
			delete mpSocket;
			mpSocket = 0;
		}
		mListening = false;
		ret = true;
	}
	return ret;
}

#endif // ML_WINDOWS