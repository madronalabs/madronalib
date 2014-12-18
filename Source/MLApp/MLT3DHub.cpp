//
//  MLT3DReceiver.cpp
//  MadronaLib
//
//  Created by Randy Jones on 11/7/14.
//
//

#include "MLT3DHub.h"

#ifdef ML_MAC

MLT3DHub::MLT3DHub() :
	mT3DWaitTime(0),
	mDataRate(-1),
	mEnabled(true),
	mUDPPortOffset(0),
	mConnected(false),
	mShouldConnect(false),
	mShouldDisconnect(false)
{
	// initialize touch frame for output
	mOutputFrame.setDims(MLT3DHub::kFrameWidth, MLT3DHub::kFrameHeight);

	// build touch frame buffer
	//
	mFrameBuf.buffer = 0;
	MLSample* pFrameData;
	pFrameData = mTouchFrames.setDims(MLT3DHub::kFrameWidth, MLT3DHub::kFrameHeight, MLT3DHub::kFrameBufferSize);
	if (pFrameData)
	{
		PaUtil_InitializeRingBuffer(&mFrameBuf, mTouchFrames.getZStride(), MLT3DHub::kFrameBufferSize, pFrameData);
	}
	else
	{
		debug() << "MLPluginProcessor::initialize: couldn't get frame data!\n";
	}

	setPortOffset(0);
	
	// start protocol polling
	startTimer(1000);
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
		nameStream << MLProjectInfo::projectName << " (" << mUDPPortOffset << ")";
		setName(nameStream.str());
		setPort(kDefaultUDPPort + mUDPPortOffset);
		
		// turn it off and back on again
		mShouldDisconnect = true;
		mShouldConnect = true;
	}
}

void MLT3DHub::setEnabled(int e)
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

void MLT3DHub::didFindService(NetServiceBrowser* pNetServiceBrowser, NetService *pNetService, bool moreServicesComing)
{
	MLNetServiceHub::didFindService(pNetServiceBrowser, pNetService, moreServicesComing);
	// debug() << "FOUND net service " << pNetService->getName() << "\n*****\n";
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

void MLT3DHub::notifyListeners(MLSymbol action, const float val)
{
	int nListeners = mpListeners.size();
	for(int i = 0; i < nListeners; ++i)
	{
		Listener* pL = mpListeners[i];
		pL->handleHubNotification(action, val);
	}
}

void MLT3DHub::ProcessMessage(const osc::ReceivedMessage& msg, const IpEndpointName&)
{
	osc::TimeTag frameTime;
	osc::int32 frameID, touchID, deviceID;
	float x, y, z, note;
    
    // todo keep track of alive touches again to fix deadbeats
	// int alive[MLProcInputToSignals::kFrameHeight] = {0};
	
	try
	{
		osc::ReceivedMessageArgumentStream args = msg.ArgumentStream();
		const char * addy = msg.AddressPattern();
        
		//debug() << "t3d: " << addy << "\n";
        
		// frame message.
		// /t3d/frm (int)frameID int)deviceID
		if (strcmp(addy, "/t3d/frm") == 0)
		{
			args >> frameID >> deviceID;
            mT3DWaitTime = 0;
			
			if(!mConnected)
			{
				mConnected = true;
				notifyListeners("connected", 1);
			}
            //debug() << "FRM " << frameID << "\n";
		}
        // match tch[n] message
        else if (strncmp(addy, "/t3d/tch", 8) == 0)
		{
            // get trailing number
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
            touchID = clamp(touchID - 1, (osc::int32)0, (osc::int32)16);
            
			// t3d/tch[ID], (float)x, (float)y, (float)z, (float)note
			args >> x >> y >> z >> note;
			
			//debug() << "TCH " << touchID << " " << x << " " << y << " " << z << " " << note << "\n";
            
			mOutputFrame(0, touchID) = x;
			mOutputFrame(1, touchID) = y;
			mOutputFrame(2, touchID) = z;
			mOutputFrame(3, touchID) = note;
		}
        
		// data rate
		else if (strcmp(addy,"/t3d/dr")==0)
		{
			osc::int32 r;
			args >> r;
			mDataRate = r;
			notifyListeners("data_rate", r);
		}
		else
		{
            //debug() << "osc unhandled:" << addy << "\n";
		}
	}
	catch( osc::Exception& e )
	{
		MLError() << "error parsing t3d message: " << e.what() << "\n";
	}
}

void MLT3DHub::ProcessBundle(const osc::ReceivedBundle& b, const IpEndpointName& remoteEndpoint)
{
	// process all messages in bundle
	//
	// ignore bundle time tag for now
	// TODO time stamping
	
	for( osc::ReceivedBundle::const_iterator i = b.ElementsBegin(); i != b.ElementsEnd(); ++i )
	{
		if( i->IsBundle() )
			ProcessBundle( osc::ReceivedBundle(*i), remoteEndpoint );
		else
			ProcessMessage( osc::ReceivedMessage(*i), remoteEndpoint );
	}
	
	// write output frame of touches to buffer
	//
	if(mFrameBuf.buffer)
		PaUtil_WriteRingBuffer(&mFrameBuf, mOutputFrame.getBuffer(), 1);
}

void MLT3DHub::timerCallback()
{
	static const int kT3DTimeout = 2; // seconds
	
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

	PollNetServices();
	
	// if we are connected and get no pings for a while, disconnect
	// assuming Soundplane or t3d device was disconnected. The plugin will be notified
	// and can revert to MIDI mode.
	if(mConnected)
	{
		// increment counter. this is reset each time we receive a t3d frame.
		mT3DWaitTime++;
		if(mT3DWaitTime > kT3DTimeout)
		{
			mConnected = false;
			notifyListeners("connected", 0);
		}
	}
}

void MLT3DHub::connect()
{
	if(listenToOSC(kDefaultUDPPort + mUDPPortOffset))
	{
		publishUDPService();
	}
}

void MLT3DHub::disconnect()
{
	if(listenToOSC(0))
	{
		removeUDPService();
	}
}

#endif // MAC