
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
#include "MLT3DPorts.h"

#include <algorithm>

//const char * kDefaultService = "default";
const char * kDomainLocal = "local.";
const char * kServiceTypeUDP = "_osc._udp";

MLNetServiceHub::MLNetServiceHub() :
	service(0),
	mPort(0)
{
	// start browsing for UDP services on all local domains
	startBrowseThread(kServiceTypeUDP);
}

MLNetServiceHub::~MLNetServiceHub()
{
	if(service) delete service;
}

void MLNetServiceHub::startBrowseThread(const char *type)
{
	browser = std::unique_ptr<NetServiceBrowser> (new NetServiceBrowser());
	browser->setListener(this);
	
	mServiceNames.clear();
	
	// launch a thread that searches for services.
	browser->searchForServicesOfType(type, kDomainLocal);
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

// for each unique service that the browser has returned, try to find the port.
// a resolver will exist if resolveWithTimeout() has been called.
void MLNetServiceHub::PollNetServices()
{
	for(auto resolver : mUniqueServices)
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
	MLConsole() << "FOUND service: " << pNetService->getDomain() << " " << pNetService->getType() << " " << pNetService->getName() << " " << pNetService->getPort() << "\n";

	auto newServiceName = pNetService->getName();
	bool found = false;
	for(auto it = mUniqueServices.begin(); it != mUniqueServices.end(); ++it)
	{
		if((*it)->getName() == newServiceName)
		{
			found = true;
			break;
		}
	}
	
	if(!found)
	{
		mUniqueServices.push_back(pNetService);
		
		// now resolve the service using PollNetServices()
		pNetService->setListener(this);
		pNetService->resolveWithTimeout(2.0, false);
	}
}

void MLNetServiceHub::didRemoveService(NetServiceBrowser *pNetServiceBrowser, NetService *pNetService, bool moreServicesComing)
{
	MLConsole() << "REMOVING service: " << pNetService->getName() << "\n";

	const std::string& nameOfServiceToDelete = pNetService->getName();

	for(auto it = mUniqueServices.begin(); it != mUniqueServices.end(); ++it)
	{
		if((*it)->getName() == nameOfServiceToDelete)
		{
			mUniqueServices.erase(it);
			break;
		}
	}
}

// called asynchronously after Resolve() when host and port are found by the resolver.
// requires that PollNetServices() be called periodically.
void MLNetServiceHub::didResolveAddress(NetService *pNetService)
{
	const std::string name = pNetService->getName();
	int port = pNetService->getPort();
	
	MLConsole() << "MLNetServiceHub::didResolveAddress: " << name << " = port " << port << "\n";
}

const std::vector<std::string>& MLNetServiceHub::getServiceNames()
{
	// push default service
	mServiceNames.clear();
	{
		std::stringstream nameStream;
		nameStream << "default" << " (" << kDefaultUDPPort << ")";
		mServiceNames.push_back(nameStream.str());
	}

	for(auto it = mUniqueServices.begin(); it != mUniqueServices.end(); ++it)
	{
		const std::string name = (*it)->getName();
		int portNum = (*it)->getPort();
		
		// if we have resolved the port for the service, push it to the list.
		if(portNum > 0)
		{
			std::stringstream nameStream;
			nameStream << name << " (" << portNum << ")";
			mServiceNames.push_back(nameStream.str());
		}
	}
	return mServiceNames;
}

std::string MLNetServiceHub::getHostName(const std::string& serviceName)
{
	std::string hostName("local."); // this may allow default port to work if resolve fails
	for(auto it = mUniqueServices.begin(); it != mUniqueServices.end(); ++it)
	{
		if(serviceName == (*it)->getName())
		{
			return (*it)->getHostName();
		}
	}
	return hostName;
}


#endif // ML_WINDOWS
