//
//  MLT3DHub.h
//  MadronaLib
//
//  Created by Randy Jones on 11/7/14.
//
//

#pragma once

#include <iostream>

//#include "MLDSPDeprecated.h"
#include "MLPlatform.h"
#include "MLOSCReceiver.h"
#include "MLNetServiceHub.h"
#include "MLT3DPorts.h"
#include "MLTimer.h"

#include "MLValue.h"
#include "MLSymbol.h"
#include "MLQueue.h"


#include <stdexcept>
#include <chrono>

#include <array>

static const int kMaxTouches = 16;
static const int kFrameBufferSize = 128;

struct Touch
{
	float x, y, z, note;
};

typedef std::array<Touch, kMaxTouches> TouchFrame;

#if defined (__APPLE__)


class MLT3DHub :
public MLNetServiceHub
{
public:
  MLT3DHub();
  ~MLT3DHub();
  
  void setEnabled(int e);
  int getPortOffset() { return mUDPPortOffset; }
  void setPortOffset(int offset);
	
	// MLNetServiceHub
  void didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing);
  
  ml::Queue<TouchFrame>* getFrameBuffer() { return &mTouchFrames; }
  
  class Listener
  {
    friend class MLT3DHub;
  public:
    Listener() {}
    virtual ~Listener() {}
  protected:
    virtual void handleHubNotification(ml::Symbol action, const ml::Value val) = 0;
  };
  
  void addListener(Listener* pL);
  void removeListener(Listener* pL);
  void notifyListeners(ml::Symbol action, const ml::Value val);
  
  void timerCallback();
  
  void setOSCPortOffset(int offset);
  
  void setShortName(const std::string& n) { mShortName = n; }
  
  osc::int32 mDataRate;
  int mT3DWaitTime;
  
private:
  void connect();
  void disconnect();
  void handleMessage(const osc::ReceivedMessage& m);
  void startBundle(const osc::ReceivedBundle& b);
  void endBundle(const osc::ReceivedBundle& b);
  
  MLOSCReceiver mOSCReceiver;
  
  // listeners to this object
  std::vector<MLT3DHub::Listener*> mpListeners;
  
  std::string mShortName; // will append a port # to this to create full name of MLNetServiceHub
  
  int mEnabled;
  int mUDPPortOffset;
  
  // true if we have actually received OSC on our connected port.
  bool mReceivingT3d;
  
  bool mConnected;
  bool mShouldConnect;
  bool mShouldDisconnect;

  ml::Queue<TouchFrame> mTouchFrames{kFrameBufferSize};
  TouchFrame mLatestTouchFrame;
	ml::Timer mTimer;
};

#endif // APPLE

