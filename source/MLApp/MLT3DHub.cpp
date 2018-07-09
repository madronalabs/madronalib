//
//  MLT3DHub.cpp
//  MadronaLib
//
//  Created by Randy Jones on 11/7/14.
//
//

#include "MLT3DHub.h"

#ifdef ML_MAC

MLT3DHub::MLT3DHub() :
mDataRate(-1),
mT3DWaitTime(0),
mEnabled(false),
mUDPPortOffset(0),
mReceivingT3d(false),
mConnected(0),
mShouldConnect(false),
mShouldDisconnect(false)
{
  setShortName("<unnamed hub>");
  
  setPortOffset(0);
  
  // start protocol polling
  startTimer(500);
}

MLT3DHub::~MLT3DHub()
{
  stopTimer();
  disconnect();
}

void MLT3DHub::setPortOffset(int offset)
{
  if(offset != mUDPPortOffset)
  {
    // set default name and port
    mUDPPortOffset = offset;
    
    std::stringstream nameStream;
    nameStream << mShortName << " (" << mUDPPortOffset << ")";
    setName(nameStream.str());
    
    setPort(kDefaultUDPPort + mUDPPortOffset);
    
    // turn it off and back on again
    mShouldDisconnect = true;
    mShouldConnect = true;
  }
}

void MLT3DHub::setEnabled(int e)
{
  if(e != mEnabled)
  {
    if(e)
    {
      mShouldConnect = true;
    }
    else
    {
      mShouldDisconnect = true;
    }
    mEnabled = e;
  }
}

void MLT3DHub::didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing)
{
  MLNetServiceHub::didFindService(pNetServiceBrowser, pNetService, moreServicesComing);
}

void MLT3DHub::addListener(MLT3DHub::Listener* pL)
{
  // TODO check for duplicates
  mpListeners.push_back(pL);
}

void MLT3DHub::removeListener(MLT3DHub::Listener* pL)
{
  int nListeners = mpListeners.size();
  for(int i = 0; i < nListeners; ++i)
  {
    Listener* pI = mpListeners[i];
    if(pL == pI)
    {
      // erase item from middle of vector and return
      for(int j = i; j < nListeners - 1; ++j)
      {
        mpListeners[j] = mpListeners[j + 1];
      }
      mpListeners.pop_back();
      return;
    }
  }
}

void MLT3DHub::notifyListeners(ml::Symbol action, const MLProperty val)
{
  int nListeners = mpListeners.size();
  for(int i = 0; i < nListeners; ++i)
  {
    Listener* pL = mpListeners[i];
    pL->handleHubNotification(action, val);
  }
}

void MLT3DHub::handleMessage(const osc::ReceivedMessage& msg)
{
  osc::TimeTag frameTime;
  osc::int32 frameID, touchID, deviceID;
  //float x, y, z, note;
  Touch newTouch;
  
  // todo keep track of alive touches again to fix deadbeats
  // int alive[MLProcInputToSignals::kFrameHeight] = {0};
  
  try
  {
    osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();
    const char * addy = msg.AddressPattern();
    
    // frame message.
    // /t3d/frm (int)frameID int)deviceID
    if (strcmp(addy, "/t3d/frm") == 0)
    {
      args >> frameID >> deviceID;
    }
    // match tch[n] message
    else if (strncmp(addy, "/t3d/tch", 8) == 0)
    {
      // get trailing number
      // TODO don't use trailing number
      touchID = 1;
      int len = strlen(addy);
      if(len == 9)
      {
        touchID = addy[8] - 48;
      }
      else if(len == 10)
      {
        touchID = 10*(addy[8] - 48) + (addy[9] - 48);
      }
      size_t touchIdx = ml::clamp(touchID - 1, (osc::int32)0, (osc::int32)16);
      
      // t3d/tch[ID], (float)x, (float)y, (float)z, (float)note
      args >> newTouch.x >> newTouch.y >> newTouch.z >> newTouch.note;
      mLatestTouchFrame[touchIdx] = newTouch;
    }
    
    // data rate message comes every second if t3d is being sent
    else if (strcmp(addy, "/t3d/dr")==0)
    {
      osc::int32 r;
      args >> r;
      mDataRate = r;
      notifyListeners("data_rate", r);
      
      mT3DWaitTime = 0;
      mReceivingT3d = true;
    }
    else if (strcmp(addy, "/pgm")==0)
    {
      osc::int32 pgm;
      args >> pgm;
      
      // debug() << "PGM " << mUDPPortOffset << ": " << pgm << "\n";
      notifyListeners("program", pgm);
    }
    else if (strcmp(addy, "/vol")==0)
    {
      float v;
      args >> v;
      
      // debug() << "VOL " << mUDPPortOffset << ": " << v << "\n";
      notifyListeners("volume", v);
    }
    
    // seq message for supporting sequencer pattern changes
    else if (strcmp(addy, "/seq")==0)
    {
      MLSignal sequence;
      sequence.setDims(16);
      osc::int32 seqWord;
      args >> seqWord;
      
      // build signal from sequence bits
      for(int i=0; i<16; ++i)
      {
        float f = (seqWord & (1<<i)) ? 1. : 0.;
        sequence[i] = f;
      }
      
      notifyListeners("sequence", sequence);
    }
  }
  catch( osc::Exception& e )
  {
    debug() << "error parsing t3d message: " << e.what() << "\n";
  }
}

void MLT3DHub::startBundle(const osc::ReceivedBundle& b)
{
  // TODO get bundle timestamp
}

void MLT3DHub::endBundle(const osc::ReceivedBundle& b)
{
  mTouchFrames.push(mLatestTouchFrame);
}

void MLT3DHub::timerCallback()
{
  static const int kT3DTimeout = 4; // in cycles of this timer
  
  if(mShouldDisconnect)
  {
    disconnect();
    mShouldDisconnect = false;
  }
  if(!mEnabled) return;
  if(mShouldConnect)
  {
    connect();
    mShouldConnect = false;
  }
  
  // if we are connected and get no pings for a while, disconnect
  // assuming Soundplane or t3d device was disconnected. The plugin will be notified
  // and can revert to MIDI mode.
  if(mReceivingT3d)
  {
    // increment counter. this is reset each time we receive a t3d frame.
    mT3DWaitTime++;
    if(mT3DWaitTime > kT3DTimeout)
    {
      mReceivingT3d = false;
    }
    notifyListeners("receiving", mReceivingT3d);
  }
}

void MLT3DHub::connect()
{
  if(!mConnected)
  {
    if(mOSCReceiver.open(kDefaultUDPPort + mUDPPortOffset))
    {
      mOSCReceiver.setBundleStartFn( [this](const osc::ReceivedBundle& b){ startBundle(b); });
      
      mOSCReceiver.setMessageFn( [this](const osc::ReceivedMessage& m){ handleMessage(m); });
      
      mOSCReceiver.setBundleEndFn( [this](const osc::ReceivedBundle& b){ endBundle(b); });
      publishUDPService();
      mConnected = true;
    }
  }
}

void MLT3DHub::disconnect()
{
  if(mConnected)
  {
    mOSCReceiver.close();
    if(mReceivingT3d)
    {
      mReceivingT3d = false;
      notifyListeners("receiving", 0);
    }
    removeUDPService();
    mConnected = false;
  }
}

#endif // MAC

