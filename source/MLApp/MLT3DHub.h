//
//  MLT3DHub.h
//  MadronaLib
//
//  Created by Randy Jones on 11/7/14.
//
//

#pragma once

#if defined (__APPLE__)

#include <iostream>

#include "MLDSPDeprecated.h"
#include "MLPlatform.h"
#include "MLOSCReceiver.h"
#include "MLNetServiceHub.h"
#include "MLDebug.h"
#include "MLSignal.h"
#include "MLSymbol.h"
#include "MLProperty.h"
#include "MLQueue.h"
#include "MLT3DPorts.h"

//#include "pthread.h"
#include <stdexcept>

#include "JuceHeader.h" // just for timer?

#include <array>

class MLT3DHub :
public MLNetServiceHub,
private juce::Timer
{
public:
  static const int kMaxTouches = 16;
  static const int kFrameBufferSize = 128;
  
  struct Touch
  {
    float x, y, z, note;
  };

  typedef std::array<Touch, kMaxTouches> TouchFrame;

  MLT3DHub();
  ~MLT3DHub();
  
  void setEnabled(int e);
  int getPortOffset() { return mUDPPortOffset; }
  void setPortOffset(int offset);
	
	// MLNetServiceHub
  void didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing);
  
  Queue<TouchFrame>* getFrameBuffer() { return &mTouchFrames; }
  
  class Listener
  {
    friend class MLT3DHub;
  public:
    Listener() {}
    virtual ~Listener() {}
  protected:
    virtual void handleHubNotification(ml::Symbol action, const MLProperty val) = 0;
  };
  
  void addListener(Listener* pL);
  void removeListener(Listener* pL);
  void notifyListeners(ml::Symbol action, const MLProperty val);
  
  void timerCallback(); // TODO no JUCE
  
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

  Queue<TouchFrame> mTouchFrames{kFrameBufferSize};
  TouchFrame mLatestTouchFrame;
};

#endif // APPLE

