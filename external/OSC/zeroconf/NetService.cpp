/*
	Copyright (c) 2009 Remy Muller. 
	
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "NetService.h"
#include "NetServiceThread.h"
#include "Thread.h"
#include <cassert>
#include <cmath>
#include <iostream>
//#include <sys/time.h>
#include <dns_sd.h>

using namespace ZeroConf;

typedef union { unsigned char b[2]; unsigned short NotAnInteger; } Opaque16;
typedef union { unsigned short NotAnInteger; unsigned char b[2]; } Opaque16b;

static void register_reply(DNSServiceRef       sdRef, 
                           DNSServiceFlags     flags, 
                           DNSServiceErrorType errorCode, 
                           const char          *name, 
                           const char          *regtype, 
                           const char          *domain, 
                           void                *context )
{
  // do something with the values that have been registered
  NetService *self = (NetService *)context;
    
  switch (errorCode)
  {
    case kDNSServiceErr_NoError:      
      if(self->getListener())
      {
        self->getListener()->didPublish(self);
      }      
      break;
    case kDNSServiceErr_NameConflict: 
      if(self->getListener())
      {
        self->getListener()->didNotPublish(self);
      }
      break;
    default:                          
      if(self->getListener())
      {
        self->getListener()->didNotPublish(self);
      }
      return;
  }
}  

static void DNSSD_API resolve_reply(DNSServiceRef client, 
                                    const DNSServiceFlags flags, 
                                    uint32_t ifIndex, 
                                    DNSServiceErrorType errorCode,
                                    const char *fullname, 
                                    const char *hosttarget, 
                                    uint16_t opaqueport, 
                                    uint16_t txtLen, 
                                    const char *txtRecord, 
                                    void *context)
{
  NetService *self = (NetService *)context;
  
  switch (errorCode)
  {
    case kDNSServiceErr_NoError:      
    {   
      Opaque16b port = { opaqueport };
      uint16_t PortAsNumber = ((uint16_t)port.b[0]) << 8 | port.b[1];
      self->setPort(PortAsNumber);
      self->setHostName(hosttarget);
        
      if(self->getListener()) 
      {  
        self->getListener()->didResolveAddress(self);
      }
      break;
    }
    default:
      break;
  }
  
  // Note: When the desired results have been returned, 
  // the client MUST terminate the resolve by calling DNSServiceRefDeallocate().
  self->stop();    
}

//------------------------------------------------------------------------------
NetService::NetService(const std::string &domain, const std::string &type, const std::string &name, const int port)
: mDNSServiceRef(NULL)
, mDomain(domain)
, mType(type)
, mName(name)
, mPort(port)
, mpListener(NULL)
, mpNetServiceThread(NULL)
{
}

NetService::NetService(const std::string &domain, const std::string &type, const std::string &name)
: mDNSServiceRef(NULL)
, mDomain(domain)
, mType(type)
, mName(name)
, mPort(-1)
, mpListener(NULL)
, mpNetServiceThread(NULL)
{
}

NetService::~NetService()
{
	stop();
}

void NetService::setListener(NetServiceListener *pNetServiceListener)
{
	mpListener = pNetServiceListener;
}

NetServiceListener *NetService::getListener() const
{
	return mpListener; 
}

void NetService::setPort(int port)
{
	mPort = port;
}

void NetService::setName(const std::string &name)
{
  mName = name;
}

void NetService::setHostName(const std::string &name)
{
  mHostName = name;
}

void NetService::publish(bool launchThread)
{
  publishWithOptions(Options(0), launchThread);
}

void NetService::publishWithOptions(Options options, bool launchThread)
{
	stop();
	if(mPort == 0)
	{
		return;
	}
	
  if(mPort < 0)
  {
    if(mpListener)
    {
      mpListener->didNotPublish(this);
    }
    return;
  } 
    
  DNSServiceFlags flags	= options;		                 // default renaming behaviour 
  uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 
  const char *name		= mName.c_str();                   /* may be NULL */
  const char *regtype		= mType.c_str(); 
  const char *domain		= mDomain.c_str();		        /* may be NULL */
  const char *host		= "";		                        /* may be NULL */
  uint16_t PortAsNumber	= mPort;
  Opaque16 registerPort   = { { static_cast<unsigned char>(PortAsNumber >> 8), static_cast<unsigned char>(PortAsNumber & 0xFF) } };
  uint16_t txtLen			= 0; 
  const void *txtRecord	= "";		                        /* may be NULL */
  DNSServiceRegisterReply callBack = (DNSServiceRegisterReply)&register_reply;	/* may be NULL */
  void *context			= this;		                        /* may be NULL */
  DNSServiceErrorType result = DNSServiceRegister(&mDNSServiceRef, 
                                                  flags, 
                                                  interfaceIndex, 
                                                  name, 
                                                  regtype, 
                                                  domain, 
                                                  host, 
                                                  registerPort.NotAnInteger,
                                                  txtLen, 
                                                  txtRecord, 
                                                  callBack, 
                                                  context);
  if(result != kDNSServiceErr_NoError)
  {
    if(mpListener)
    {
      mpListener->didNotPublish(this);
    }
		if(mDNSServiceRef)
			DNSServiceRefDeallocate(mDNSServiceRef);
		mDNSServiceRef = NULL;			
  }
  else
  {
    if(mpListener)
    {
      mpListener->willPublish(this);
    }
		if(launchThread)
		{
			mpNetServiceThread = new NetServiceThread(mDNSServiceRef, 1.0);
			mpNetServiceThread->startThread();
		}
  }  
}

void NetService::resolveWithTimeout(double timeOutInSeconds, bool launchThread)
{
	stop();
	
  DNSServiceFlags flags	= 0;
  uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 
  DNSServiceErrorType err = DNSServiceResolve(&mDNSServiceRef,
                                              flags,
                                              interfaceIndex,
                                              mName.c_str(),
                                              mType.c_str(),
                                              mDomain.c_str(),
                                              (DNSServiceResolveReply)&resolve_reply,
                                              this);
  
  if (!mDNSServiceRef || err != kDNSServiceErr_NoError) 
  { 
    if(mpListener)
    {
      mpListener->didNotResolve(this);
    }
		if(mDNSServiceRef)
			DNSServiceRefDeallocate(mDNSServiceRef);
		mDNSServiceRef = NULL;			
  }
  else
  {
    if(mpListener)
    {
      mpListener->willResolve(this);
    }
		if(launchThread)
		{
			mpNetServiceThread = new NetServiceThread(mDNSServiceRef, 1.0);
			mpNetServiceThread->startThread();
		}
  }
}

void NetService::stop()
{
  if(mpNetServiceThread)
  {
    mpNetServiceThread->setThreadShouldExit();
    mpNetServiceThread->waitForThreadToExit(1000);
    delete mpNetServiceThread;
    mpNetServiceThread = NULL;
  }
	
	if(mDNSServiceRef)
	{
		DNSServiceRefDeallocate(mDNSServiceRef);
		mDNSServiceRef = NULL;
	}	
}

void NetService::startMonitoring()
{
	assert(0);
}

void NetService::stopMonitoring()
{
	assert(0);
}
