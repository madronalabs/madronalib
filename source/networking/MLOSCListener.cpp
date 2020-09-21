
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPlatform.h"

#if ML_WINDOWS
// TODO
#else

#include "MLOSCListener.h"

MLOSCListener::MLOSCListener() : mListening(false), mpSocket(0), mPort(0), mListenerThread(nullptr)
{
}

MLOSCListener::~MLOSCListener()
{
  // MLConsole() << "deleting  MLOSCListener\n";
  listenToOSC(0);
}

void* MLOSCListenerStartThread(void* arg)
{
  MLOSCListener* pL = static_cast<MLOSCListener*>(arg);
  UdpListeningReceiveSocket* pSocket = pL->mpSocket;

  try
  {
    // MLConsole() << "MLOSCListener running socket. \n";
    pSocket->Run();
  }
  catch (osc::Exception& e)
  {
    // MLConsole() << "MLOSCListener caught osc exception: " << e.what() <<
    // "\n";
  }
  catch (std::runtime_error& e)
  {
    // MLConsole() << "MLOSCListener caught runtime_error exception: " <<
    // e.what() << "\n";
  }

  // MLOSCListener* pListener = static_cast<MLOSCListener*>(arg);

  // // MLConsole() << "MLOSCListener: listener thread on port " <<
  // pListener->getPort() << " terminated.\n";
  return 0;
}

int MLOSCListener::listenToOSC(int port)
{
  int ret = false;
  if (port)
  {
    if (mpSocket)
    {
      listenToOSC(0);
    }
    try
    {
      // // MLConsole() << "MLOSCListener: trying listen on port " << port <<
      // "...\n";
      mpSocket =
          new UdpListeningReceiveSocket(IpEndpointName(IpEndpointName::ANY_ADDRESS, port), this);
    }
    catch (osc::Exception& e)
    {
      mpSocket = 0;
      // MLConsole() << "MLOSCListener::listenToOSC: couldn't bind to port " <<
      // port << ".\n"; MLConsole() << "error: " << e.what() << "\n";
    }
    catch (...)
    {
      mpSocket = 0;
      // MLConsole() << "MLOSCListener::listenToOSC: couldn't bind to port " <<
      // port << ".\n"; MLConsole() << "Unknown error.\n";
    }

    if (mpSocket)
    {
      // MLConsole() << "MLOSCListener::listenToOSC: created receive socket on
      // port " << port << ".\n";
      mPort = port;

      int err;
      pthread_attr_t attr;

      // MLConsole() << "initializing pthread attributes...\n";
      err = pthread_attr_init(&attr);

      if (!err)
      {
        // MLConsole() << "creating listener thread...\n";
        err = pthread_create(&mListenerThread, &attr, &MLOSCListenerStartThread, (void*)this);

        if (!err)
        {
          ret = true;
          mListening = true;
        }
      }
    }
  }
  else
  {
    if (mpSocket)
    {
      // MLConsole() << "MLOSCListener: disconnecting.\n";
      mpSocket->Break();
      delete mpSocket;
      mpSocket = 0;
    }
    if (mListenerThread != nullptr)
    {
      // MLConsole() << "MLOSCListener: cancelling listener thread.\n";
      pthread_cancel(mListenerThread);
      mListenerThread = nullptr;
    }
    mListening = false;
    ret = true;
  }
  return ret;
}

#endif  // ML_WINDOWS
