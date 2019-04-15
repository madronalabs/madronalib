
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#if ML_WINDOWS
	// TODO
#else

#include "NetService.h"
#include "NetServiceBrowser.h"

#include <cassert>
#include <math.h>
#include <vector>
#include <string>
#include <sstream>
#include <list>
#include <memory>
#include <sys/select.h>

using ZeroConf::NetServiceListener;
using ZeroConf::NetServiceBrowserListener;
using ZeroConf::NetService;
using ZeroConf::NetServiceBrowser;

extern const char * kDomainLocal;
extern const char * kServiceTypeUDP;

class MLNetServiceHub : 
	public NetServiceListener, 
	public NetServiceBrowserListener
{
public:
	std::unique_ptr<NetServiceBrowser> browser;
	NetService *service;

	MLNetServiceHub();
	~MLNetServiceHub();

	virtual void startBrowseThread(const char *type);
	
	virtual void publishUDPService();
	virtual void removeUDPService();
	
	void setName(const std::string& name);
	void setPort(int port);

	bool pollService(DNSServiceRef dnsServiceRef, double timeOutInSeconds, DNSServiceErrorType &err);
	void PollNetServices();
	
	const std::vector<std::string>& getFormattedServiceNames();
	std::string unformatServiceName(const std::string& formattedServiceName);
	std::string getHostName(const std::string& serviceName);
	int getPort(const std::string& serviceName);

protected:
	
	// ZeroConf::NetServiceListener
	void willPublish(NetService *) {}
	void didNotPublish(NetService *) {}
	void didPublish(NetService *pNetService) {}
	
	void willResolve(NetService *) {}
	void didNotResolve(NetService *) {}
	void didResolveAddress(NetService *pNetService);
	
	void didUpdateTXTRecordData(NetService *) {}
	void didStop(NetService *) {}
	
	// ZeroConf::NetServiceBrowserListener
	void didFindDomain(NetServiceBrowser *, const std::string &, bool ) {}
	void didRemoveDomain(NetServiceBrowser *, const std::string &, bool ) {}

	void didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing);
	void didRemoveService(NetServiceBrowser *pNetServiceBrowser, NetService *pNetService, bool moreServicesComing);

	void willSearch(NetServiceBrowser *) {}
	void didNotSearch(NetServiceBrowser *) {}
	void didStopSearch(NetServiceBrowser *) {}

	std::vector<NetService*> mUniqueServices;
	std::vector<std::string> mServiceNames;
	std::string mName;
	int mPort;
};

#endif // ML_WINDOWS

