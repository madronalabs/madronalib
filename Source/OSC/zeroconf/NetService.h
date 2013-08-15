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

#ifndef ZeroConf_NetService_H
#define ZeroConf_NetService_H

#include <string>
#include "zeroconf/CriticalSection.h"
#include "dns_sd.h"

namespace ZeroConf 
{
	class NetService;
  class NetServiceThread;
	
	class NetServiceListener
	{
	public:
		virtual ~NetServiceListener() {}
		
		virtual void willPublish(NetService *pNetService) = 0;
		virtual void didNotPublish(NetService *pNetService) = 0;
		virtual void didPublish(NetService *pNetService) = 0;
		
		virtual void willResolve(NetService *pNetService) = 0;
		virtual void didNotResolve(NetService *NetService) = 0;
		virtual void didResolveAddress(NetService *pNetService) = 0;
		
		virtual void didUpdateTXTRecordData(NetService *pNetService) = 0;
		
		virtual void didStop(NetService *pNetService) = 0;
	};
		
	/**
	  C++ equivalent to the Cocoa API class: NSNetService.
	 cf http://developer.apple.com/documentation/Cocoa/Reference/Foundation/Classes/NSNetService_Class/Reference/Reference.html
	 
	 The NetService class represents a network service that your application publishes or uses as a client. 
	 This class and the NetServiceBrowser class use multicast DNS to convey information about network services to and from your application. 
	 The API of NetService provides a convenient way to publish the services offered by your application and to resolve the socket address for a service.
	 
	 The types of services you access using NetService are the same types that you access directly using BSD sockets. 
	 HTTP and FTP are two services commonly provided by systems. 
	 (For a list of common services and the ports used by those services, see the file /etc/services.) 
	 Applications can also define their own custom services to provide specific data to clients.
	 
	 You can use the NetService class as either a publisher of a service or as a client of a service. 
	 If your application publishes a service, your code must acquire a port and prepare a socket to communicate with clients. 
	 Once your socket is ready, you use the NetService class to notify clients that your service is ready. 
	 If your application is the client of a network service, you can either create an NetService object directly 
	 (if you know the exact host and port information) or you can use an NetServiceBrowser object to browse for services.
	 
	 To publish a service, you must initialize your NetService object with the service name, domain, type, and port information. 
	 All of this information must be valid for the socket created by your application. 
	 Once initialized, you call the publish method to broadcast your service information out to the network.
	 
	 When connecting to a service, you would normally use the NetServiceBrowser class to locate the service on the network and obtain the corresponding NSNetService object. 
	 Once you have the object, you proceed to call the resolveWithTimeout: method to verify that the service is available and ready for your application. 
	 If it is, the addresses method returns the socket information you can use to connect to the service.
	 
	 The methods of NetService operate asynchronously so that your application is not impacted by the speed of the network. 
	 All information about a service is returned to your application through the NetService object’s listener. 
	 You must provide a listener object to respond to messages and to handle errors appropriately.		
	 */
	class NetService
	{
	public:
		enum { NoAutoRename = 1<<0 };
		typedef unsigned int Options; ///< These constants specify options for a network service.
		
		/**
		 Constructor used when publishing services.
		 */
		NetService(const std::string &domain, const std::string &type, const std::string &name, const int port);
		
		/**
		 Constructor used when resolving services.
		 
		 This method is the appropriate initializer to use to resolve a service.
		 If you know the values for domain, type, and name of the service you wish to connect to, you can create an NSNetService object using this initializer and call resolveWithTimeout: on the result.		 
		 You cannot use this initializer to publish a service. 
		 This initializer passes an invalid port number, which prevents the service from being registered. 
		 Calling publish on an NetService object initialized with this method generates a call to your listener’s netService:didNotPublish: method with an NSNetServicesBadArgumentError error.		 
		 */
		NetService(const std::string &domain, const std::string &type, const std::string &name);
		virtual ~NetService();
		
		void setListener(NetServiceListener *pNetServiceListener);
		NetServiceListener *getListener() const;
		
		void publish(bool launchThread);
		void publishWithOptions(Options options, bool launchThread=true);
		
		void resolveWithTimeout(double timeOutInSeconds, bool launchThread = true);

		/** Halts a currently running attempt to publish or resolve a service. 
		 
		 this method results in the sending of a netServiceDidStop: message to the delegate.
		 */
		void stop();
		
		void startMonitoring();
		void stopMonitoring();
		
		const std::string& getDomain() const { return mDomain; }
		
    const std::string& getType() const { return mType; }

    void setName(const std::string &name);
		const std::string& getName() const { return mName; }
    
    void setPort(int port);
		const int getPort() const {return mPort; }

    void setHostName(const std::string &name);
		const std::string& getHostName() const { return mHostName; }

		DNSServiceRef getDNSServiceRef() { return mDNSServiceRef; }
		
	private:
		DNSServiceRef mDNSServiceRef;
		std::string mDomain;
		std::string mType;
		std::string mName;
    std::string mHostName;
		int mPort;
		std::string mTXTRecordData;
		NetServiceListener *mpListener;
		CriticalSection mCriticalSection;
    
    NetServiceThread *mpNetServiceThread;
	};
}

#endif