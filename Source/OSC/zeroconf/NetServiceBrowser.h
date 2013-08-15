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

#ifndef ZeroConf_NetServiceBrowser_H
#define ZeroConf_NetServiceBrowser_H

#include "zeroconf/CriticalSection.h"
#include <string>
#include <vector>
#include "dns_sd.h"

namespace ZeroConf
{
	class NetService;
	class NetServiceBrowser;
  class NetServiceThread;
	
	class NetServiceBrowserListener
	{
	public:
		virtual ~NetServiceBrowserListener() {}
		
		virtual void didFindDomain(NetServiceBrowser *pNetServiceBrowser, const std::string &domainName, bool moreDomainsComing) = 0;
		virtual void didRemoveDomain(NetServiceBrowser *pNetServiceBrowser, const std::string &domainName, bool moreDomainsComing) = 0;

		virtual void didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing) = 0;
		virtual void didRemoveService(NetServiceBrowser *pNetServiceBrowser, NetService *pNetService, bool moreServicesComing) = 0;
		
		virtual void willSearch(NetServiceBrowser *pNetServiceBrowser) = 0;
		virtual void didNotSearch(NetServiceBrowser *pNetServiceBrowser) = 0;
		
		virtual void didStopSearch(NetServiceBrowser *pNetServiceBrowser) = 0;
	};
	
	/** C++ equivalent to Cocoa NSNetServiceBrowser
	 cf http://developer.apple.com/documentation/Cocoa/Reference/Foundation/Classes/NSNetServiceBrowser_Class/Reference/Reference.html
	 
	 The NetServiceBrowser class defines an interface for finding published services on a network using multicast DNS. 
	 An instance of NetServiceBrowser is known as a network service browser.
	 
	 Services can range from standard services, such as HTTP and FTP, to custom services defined by other applications. 
	 You can use a network service browser in your code to obtain the list of accessible domains and then to obtain an NetService object for each discovered service. 
	 Each network service browser performs one search at a time, so if you want to perform multiple simultaneous searches, use multiple network service browsers.
	 
	 A network service browser performs all searches asynchronously using the current run loop to execute the search in the background. 
	 Results from a search are returned through the associated listener object, which your client application must provide. 
	 Searching proceeds in the background until the object receives a stop message.
	 
	 To use an NetServiceBrowser object to search for services, allocate it, initialize it, and assign a listener. 
	 Once your object is ready, you begin by gathering the list of accessible domains using either the searchForRegistrationDomains or searchForBrowsableDomains methods. 
	 From the list of returned domains, you can pick one and use the searchForServicesOfType method to search for services in that domain.
	 
	 The NetServiceBrowser class provides two ways to search for domains. 
	 In most cases, your client should use the searchForRegistrationDomains method to search only for local domains to which the host machine has registration authority. 
	 This is the preferred method for accessing domains as it guarantees that the host machine can connect to services in the returned domains. 
	 Access to domains outside this list may be more limited.
	 */
	class NetServiceBrowser
	{
	public:
		NetServiceBrowser();
		virtual ~NetServiceBrowser();
		
		void setListener(NetServiceBrowserListener *pNetServiceBrowserListener);
		NetServiceBrowserListener* getListener() const;

		void searchForBrowsableDomains();
		void searchForRegistrationDomains();
		
		void searchForServicesOfType(const std::string &serviceType, const std::string &domainName, bool launchThread=true);
		
		/** Halts a currently running search or resolution. */
		void stop();
		
    void addService(const char *domain, const char *type, const char *name, bool moreComing);
    void removeService(const char *domain, const char *type, const char *name, bool moreComming);
    
		DNSServiceRef getDNSServiceRef() { return mDNSServiceRef; }
		
	private:
		DNSServiceRef mDNSServiceRef;
		NetServiceBrowserListener *mpListener;
		CriticalSection mCriticalSection;
    NetServiceThread *mpNetServiceThread;
    std::vector<NetService*> mServices;
	};
}

#endif