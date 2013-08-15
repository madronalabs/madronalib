#include "NetServiceThread.h"
#include <cassert>
#include <cmath>
#include <iostream>
//#include <sys/time.h>

using namespace ZeroConf;

NetServiceThread::NetServiceThread(DNSServiceRef dnsServiceRef, double timeOutInSeconds)
: mDNSServiceRef(dnsServiceRef)
, mTimeOut(timeOutInSeconds)
{
}

NetServiceThread::~NetServiceThread()
{
  if(!waitForThreadToExit(100))
  {
    stopThread(1);
  }
}

bool NetServiceThread::poll(DNSServiceRef dnsServiceRef, double timeOutInSeconds, DNSServiceErrorType &err)
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

void NetServiceThread::run()
{
	std::cout << "NetServiceThread::start()" << std::endl;

	while (!threadShouldExit()) 
	{
		DNSServiceErrorType err = kDNSServiceErr_NoError;
		if(poll(mDNSServiceRef, mTimeOut, err))
		{
			if(err>0)
			{
				setThreadShouldExit();
			}
		}
	}
	std::cout << "NetServiceThread::stop()" << std::endl;
}
