
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPlatform.h"

#if ML_WINDOWS
	// TODO
#else
	
#include "MLOSCListener.h"

MLOSCListener::MLOSCListener() :
	mpSocket(0),
	mSocketActive(0),
	mPort(0),
	mListening(false)
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
	int port = pL->mPort;
			
	debug() << "MLOSCListener: starting listen thread for OSC on port " << port << "...\n";

	try
	{
		pSocket->Run();
	}
	catch( osc::Exception& e )
	{
		MLError() << "MLOSCListener caught osc exception: " << e.what() << "\n";
	}
	catch( std::runtime_error& e )
	{
		MLError() << "MLOSCListener caught runtime_error exception: " << e.what() << "\n";
	}
	
	// debug() << "MLOSCListener: listener thread on port " << port << " terminated.\n";

	pL->mSocketActive = false;
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
			debug() << "MLOSCListener: trying listen on port " << port << "...\n";
			mpSocket = new UdpListeningReceiveSocket(
				IpEndpointName( IpEndpointName::ANY_ADDRESS, port), 
				this);
		}
		catch( osc::Exception& e )
		{
			// TODO find another socket!  Integrate with zeroconf stuff.  
			
			// TODO MLError() should do what exactly?
			mpSocket = 0;
			debug() << "MLOSCListener::listenToOSC: couldn't bind to port " << port << ".\n";
			debug() << "error: " << e.what() << "\n";
		}
		catch(...)
		{
			mpSocket = 0;
			debug() << "MLOSCListener::listenToOSC: couldn't bind to port " << port << ".\n";
			debug() << "Unknown error.\n";
		}
		
		if(mpSocket)
		{
			debug() << "MLOSCListener::listenToOSC: created receive socket on port " << port << ".\n";
			mSocketActive = true;
			mPort = port;
			
			int err;
			pthread_attr_t attr;
			
			// debug() << "initializing pthread attributes...\n";
			err = pthread_attr_init(&attr);

			if(!err)
			{
				// debug() << "creating listener thread...\n";
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
		mListening = false;
		if(mpSocket)
		{
			mpSocket->AsynchronousBreak();
			while(mSocketActive)
			{
				usleep(100);
			}
			delete mpSocket;
			mpSocket = 0;
			mPort = 0;
			ret = true;
		}
	}
	return ret;
}

#endif // ML_WINDOWS