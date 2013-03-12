/*
 *  MLProcOSCToSignals.h
 *  AaltoForSoundplane
 *
 *  Created by Randy Jones on 2/24/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef ML_PROC_OSC_TO_SIGNALS_H
#define ML_PROC_OSC_TO_SIGNALS_H

#include <iostream>
#include <cstring>

#include "MLDSP.h"
#include "MLProc.h"
#include "MLScale.h"

#include "pa_ringbuffer.h"

#include "OscReceivedElements.h"
#include "OscPacketListener.h"
#include "UdpSocket.h"

using namespace osc;

//#define MAX_CENTROIDS 8		// fixed number of outputs

#define PORT 7000

const int kOSCToSignalsFrameWidth = 4;
const int kOSCToSignalsFrameHeight = 16;
const int kOSCToSignalsFrameBufferSize = 128;

#pragma mark OSC listener

class SoundplaneOSCListener : public osc::OscPacketListener 
{
public:
	SoundplaneOSCListener();
	~SoundplaneOSCListener();	
	void setOutputBuffer(PaUtilRingBuffer* pBuf);
	
protected:
    virtual void ProcessMessage( const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint );
				
private:
	PaUtilRingBuffer* mpOutputBuffer;
};

class MLOSCVoice
{
public:
	MLOSCVoice();
	~MLOSCVoice() {} ;
	
	void clear();
	MLProc::err resize(int size);

	MLSignal mPitch;
	MLSignal mPressure;
	MLSignal mDx;
	MLSignal mDy;
	
	float mStartX;
	float mStartY;
	float mX1;
	float mY1;
	float mZ1;
	
	bool mActive;
};

extern const int kNumOSCVoiceSignals;
extern const char * OSCVoiceSignalNames[];

class MLProcOSCToSignals : public MLProc
{
public:

	 MLProcOSCToSignals();
	~MLProcOSCToSignals();

	void clear();
	MLProc::err prepareToProcess();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
 	void setup();
 	err resize();
	int getOutputIndex(const MLSymbol name);
		
	MLScale* getScale();
	MLSample xyToPitch(float x, float y);
	MLSample xToDx(float x);
	MLSample yToDy(float y);
	MLSample zToPressure(float z);

	void doParams();
	PaUtilRingBuffer* getFrameBuffer() { return &mFrameBuf; }
	
private:
	MLProcInfo<MLProcOSCToSignals> mInfo;
	
	int mCurrentVoices;
	int mTest;

	void dumpEvents();
	void dumpVoices();
	
	MLOSCVoice mVoices[kMLEngineMaxVoices]; 

	PaUtilRingBuffer mFrameBuf;
	int mFrameCount;
	
	MLSignal mTouchFrames;
	MLSignal mLatestFrame;

	pthread_t mListenerThread;

	int temp;
	float mNoteTable[128];
};

void *OSCListenerThread(void *arg);

#endif // ML_PROC_OSC_TO_SIGNALS_H