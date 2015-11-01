
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

/*
	Portions adapted from Oscbonjour:
	Copyright (c) 2005-2009 RÈmy Muller. 
	
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

#include "MLPlatform.h"

#if ML_WINDOWS
	// TODO
#else

#include "MLNetServiceHub.h"

#include <algorithm>

const char * kDomainLocal = "local.";
const char * kServiceTypeUDP = "_osc._udp";

MLNetServiceHub::MLNetServiceHub() :
	browser(0),
	resolver(0),
	service(0),
	mPort(0)
{
}

MLNetServiceHub::~MLNetServiceHub()
{
	if(browser) delete browser;
	if(resolver)delete resolver;
	if(service) delete service;
}

void MLNetServiceHub::Browse(const char *domain, const char *type)
{
	if(browser) delete browser;
	browser = 0;
	browser = new NetServiceBrowser();
	browser->setListener(this);
	
	// launch a thread that searches for services.
	browser->searchForServicesOfType(type, domain);
}

void MLNetServiceHub::Resolve(const char *domain, const char *type, const char *name)
{
	if(resolver) delete resolver;
	resolver = 0;
	resolver = new NetService(domain,type,name);
	resolver->setListener(this);
	resolver->resolveWithTimeout(10.0, false);  // ML temp
}

bool MLNetServiceHub::pollService(DNSServiceRef dnsServiceRef, double timeOutInSeconds, DNSServiceErrorType &err)
{
	assert(dnsServiceRef);

	err = kDNSServiceErr_NoError;
	
  fd_set readfds;
	FD_ZERO(&readfds);
	
  int dns_sd_fd = DNSServiceRefSockFD(dnsServiceRef);
  int nfds = dns_sd_fd+1;
	FD_SET(dns_sd_fd, &readfds);
	
  struct timeval tv;
  tv.tv_sec = long(floor(timeOutInSeconds));
  tv.tv_usec = long(1000000*(timeOutInSeconds - tv.tv_sec));
	
	int result = select(nfds,&readfds,NULL,NULL,&tv);
	if(result>0 && FD_ISSET(dns_sd_fd, &readfds))
	{
		err = DNSServiceProcessResult(dnsServiceRef);
		return true;
	}
	
	return false;
}

void MLNetServiceHub::PollNetServices()
{
	if(resolver && resolver->getDNSServiceRef())
	{
		DNSServiceErrorType err = kDNSServiceErr_NoError;
		if(pollService(resolver->getDNSServiceRef(), 0.001, err))
		{
			resolver->stop();
		}
	}
}

void MLNetServiceHub::setName(const std::string& name)
{
	mName = std::string(name);
}

void MLNetServiceHub::setPort(int port)
{
	mPort = port;
	if(service)
	{
		service->setName(mName);
		service->setPort(port);
		service->publish(false);
	}
}

void MLNetServiceHub::publishUDPService()
{
	service = new NetService(kDomainLocal, kServiceTypeUDP, mName.c_str(), mPort);
	service->setListener(this);
	service->publish(false);
}

void MLNetServiceHub::removeUDPService()
{
	if(service)
	{
		delete service;
		service = 0;
	}
}

void MLNetServiceHub::didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing)
{
	// debug() << "FOUND service: " << pNetService->getName() << "\n";

	veciterator it = std::find(mServices.begin(),mServices.end(), pNetService->getName());
	if(it!=mServices.end()) return; // we already have it
	mServices.push_back(pNetService->getName());
}

void MLNetServiceHub::didRemoveService(NetServiceBrowser *pNetServiceBrowser, NetService *pNetService, bool moreServicesComing)
{
	// debug() << "REMOVED service: " << pNetService->getName() << "\n";

	veciterator it = std::find(mServices.begin(),mServices.end(), pNetService->getName());
	if(it==mServices.end()) return;      // we don't have it
	//long index = it-mServices.begin();   // store the position
	mServices.erase(it);
}

void MLNetServiceHub::didResolveAddress(NetService *pNetService)
{
//	const std::string& hostName = pNetService->getHostName();
//	int port = pNetService->getPort();
}

void MLNetServiceHub::didPublish(NetService *pNetService)
{
}

#endif // ML_WINDOWS
