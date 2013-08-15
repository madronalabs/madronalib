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

#include "zeroconf/NetServiceBrowser.h"
#include "zeroconf/NetService.h"
#include "zeroconf/NetServiceThread.h"
#include <cassert>
#include <dns_sd.h>

using namespace ZeroConf;

static void browse_reply(DNSServiceRef client, 
                         const DNSServiceFlags flags,
                         uint32_t ifIndex,                          
                         DNSServiceErrorType errorCode,
                         const char *replyName, 
                         const char *replyType, 
                         const char *replyDomain,                             
                         void *context)
{
  NetServiceBrowser *self = (NetServiceBrowser *)context;
	const bool moreComing = bool(flags & kDNSServiceFlagsMoreComing);
  
  if(!self->getListener()) return;
  
  if(flags & kDNSServiceFlagsAdd)
  {
    self->addService(replyDomain,replyType,replyName, moreComing);
  }
  else
  {
    self->removeService(replyDomain,replyType,replyName, moreComing);
  }
}

NetServiceBrowser::NetServiceBrowser()
: mpListener(NULL)
, mpNetServiceThread(NULL)
, mDNSServiceRef(NULL)
{
}

NetServiceBrowser::~NetServiceBrowser()
{
	stop();
}

void NetServiceBrowser::setListener(NetServiceBrowserListener *pNetServiceBrowserListener)
{
	mpListener = pNetServiceBrowserListener;
}

NetServiceBrowserListener* NetServiceBrowser::getListener() const
{
	return mpListener;
}

void NetServiceBrowser::searchForBrowsableDomains()
{
	assert(0);
}

void NetServiceBrowser::searchForRegistrationDomains()
{
	assert(0);
}

void NetServiceBrowser::searchForServicesOfType(const std::string &serviceType, const std::string &domainName, bool launchThread)
{
	stop();
	  
  DNSServiceFlags flags	= 0;		// default renaming behaviour 
  uint32_t interfaceIndex = kDNSServiceInterfaceIndexAny;		// all interfaces 
  DNSServiceErrorType err = DNSServiceBrowse(&mDNSServiceRef, 
                                             flags, 
                                             interfaceIndex, 
                                             serviceType.c_str(), 
                                             domainName.c_str(), 
                                             (DNSServiceBrowseReply)&browse_reply, 
                                             this);
  
  if (!mDNSServiceRef || err != kDNSServiceErr_NoError) 
  { 
    if(mpListener)
    {
      mpListener->didNotSearch(this);
    }
		if(mDNSServiceRef)
			DNSServiceRefDeallocate(mDNSServiceRef);
		mDNSServiceRef = NULL;
  }
  else
  {
    if(mpListener)
       mpListener->willSearch(this);

		if(launchThread)
		{
			mpNetServiceThread = new NetServiceThread(mDNSServiceRef, 1.0);
			mpNetServiceThread->startThread();
		}
  }
}

void NetServiceBrowser::stop()
{
  if(mpNetServiceThread)
  {
    mpNetServiceThread->setThreadShouldExit();
    mpNetServiceThread->waitForThreadToExit(100);
    delete mpNetServiceThread;
    mpNetServiceThread = NULL;
  }
	
	if(mDNSServiceRef)
		DNSServiceRefDeallocate(mDNSServiceRef);
	mDNSServiceRef = NULL;
}

void NetServiceBrowser::addService(const char *domain, const char *type, const char *name, bool moreComing)
{
  NetService *pNetService = new NetService(domain, type, name);
  {
    ScopedLock lock(mCriticalSection);
    mServices.push_back(pNetService);     
  }
  if(mpListener)
    mpListener->didFindService(this, pNetService, moreComing);
}

void NetServiceBrowser::removeService(const char *domain, const char *type, const char *name, bool moreComing)
{
  ScopedLock lock(mCriticalSection);
  for(std::vector<NetService*>::iterator it = mServices.begin(); it != mServices.end();)
  {
       if((*it)->getName() == name && (*it)->getDomain() == domain && (*it)-> getType() == type)
       {
          NetService *pNetService = *it;
          it = mServices.erase(it);
          if(mpListener)
            mpListener->didRemoveService(this, pNetService, moreComing);
       }
       else
       {
        ++it;
       }
  }
}

