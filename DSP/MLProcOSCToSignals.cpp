/*
 *  MLProcOSCToSignals.cpp
 *  AaltoForSoundplane
 *
 *  Created by Randy Jones on 2/24/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "MLProcOSCToSignals.h"
 
const int kNumOSCVoiceSignals = 6;
const char * OSCVoiceSignalNames[kNumOSCVoiceSignals] = 
{
	"pitch",
	"amp",
	"pressure",
	"voice",
	"dx",
	"dy"
};

//--------------------------------------------------------------------------------
#pragma mark OSC listener

SoundplaneOSCListener::SoundplaneOSCListener() :
	mpOutputBuffer(0)
	{}
	
SoundplaneOSCListener::~SoundplaneOSCListener(){}	

void SoundplaneOSCListener::setOutputBuffer(PaUtilRingBuffer* pBuf)
{
	mpOutputBuffer = pBuf;
}

void SoundplaneOSCListener::ProcessMessage( const osc::ReceivedMessage& msg, 
			const IpEndpointName& remoteEndpoint )
{
	try 
	{
		ReceivedMessageArgumentStream args = msg.ArgumentStream();
		ReceivedMessage::const_iterator arg = msg.ArgumentsBegin();
		
		if( strcmp( msg.AddressPattern(), "/tuio2" ) == 0 )
		{
			const char* cmd;
			args >> cmd;
			
			if (strcmp(cmd,"frm")==0) 
			{	
				// frame message, read time and 
				TimeTag frameTime;
				int32 frameId;
				args >> frameId >> frameTime; 

			} 
			else if (strcmp(cmd,"ptr")==0) 
			{
				// pointer 
				
			}
			else if (strcmp(cmd,"alv")==0) 
			{
				// alive message
				
			}
		}
	}
	catch( Exception& e )
	{
		std::cerr << "error parsing TUIO message: " << e.what() << "\n";
	}
		
									//
	// /tuio2/ptr s_id tu_id c_id x_pos y_pos width press [x_vel y_vel m_acc]
	
		
		
		/*
		// arguments to message should be [x, y, z, status] * number of centroids
		// so fill the centroids in the incoming frame one after another
		for(int i=0; i<MAX_CENTROIDS; ++i)
		{
			args >> x >> y >> z >> age;
			mIncomingFrame.mSamples[i].set(x, y, z, age);
		}
		*/
		
//		args >> osc::EndMessage;
//		PaUtil_WriteRingBuffer(mpOutputBuffer, &mIncomingFrame, 1);	
	
}

//--------------------------------------------------------------------------------
#pragma mark MLOSCVoice

MLOSCVoice::MLOSCVoice() :
	mStartX(0.),
	mStartY(0.),
	mX1(0.),
	mY1(0.),
	mZ1(0.),
	mActive(false)
{
	clear();
}

MLProc::err MLOSCVoice::resize(int bufSize)
{
	MLProc::err ret = MLProc::OK;

	// make delta lists
	// allow for one change each sample, though this is unlikely to get used.
	MLSample* a = mPitch.setDims(bufSize);	
	MLSample* b = mPressure.setDims(bufSize);	
	MLSample* c = mDx.setDims(bufSize);	
	MLSample* d = mDy.setDims(bufSize);	
	
	if (a && b && c && d)
	{
		ret = MLProc::OK;
	}
	else
	{
		ret = MLProc::memErr;
	}

	return ret;
}

void MLOSCVoice::clear()
{
	mPitch.clear();
	mPressure.clear();
	mDx.clear();
	mDy.clear();
}

//--------------------------------------------------------------------------------
#pragma mark MLProcOSCToSignals

namespace
{
	MLProcRegistryEntry<MLProcOSCToSignals> classReg("osc_to_signals");
	ML_UNUSED MLProcParam<MLProcOSCToSignals> params[2] = { "bufsize", "voices" };
	// no input signals.
	ML_UNUSED MLProcOutput<MLProcOSCToSignals> outputs[] = {"*"};	// variable outputs
}	

MLProcOSCToSignals::MLProcOSCToSignals() :
	mCurrentVoices(0),
	mTest(0),
	mFrameCount(0)
{
//	debug() << "MLProcOSCToSignals constructor:\n";
	setParam("voices", 0);	// default
	
	temp = 0;

	//get notes TEMP
	// MIDI notes to log pitch ratios
	for(int i=0; i<128; ++i)
	{
		float f = i;
		mNoteTable[i] = f/12.f - 2.f;
	}
}

MLProcOSCToSignals::~MLProcOSCToSignals()
{
//	debug() << "MLProcOSCToSignals destructor\n";

}

MLProc::err MLProcOSCToSignals::resize() 
{
	MLSample* pFrameData;
	MLProc::err ret = OK;
	if (mParamsChanged) doParams(); 
	
	int bufSize = (int)getParam("bufsize");
//	int vecSize = getContextVectorSize();
//	MLSampleRate rate = getCoqntextSampleRate();

	pFrameData = mTouchFrames.setDims(kOSCToSignalsFrameWidth, kOSCToSignalsFrameHeight, kOSCToSignalsFrameBufferSize);
	if (pFrameData)
	{
		PaUtil_InitializeRingBuffer(&mFrameBuf, mTouchFrames.getZStride(), kOSCToSignalsFrameBufferSize, pFrameData);
	}
	else
	{
		return MLProc::memErr;
	}
	
	if (!mLatestFrame.setDims(kOSCToSignalsFrameWidth, kOSCToSignalsFrameHeight))
	{
		return MLProc::memErr;
	}
	
	int err;
	pthread_attr_t attr;
//	struct sched_param param;
	err = pthread_attr_init(&attr);
	assert(!err);
//	err = pthread_attr_getschedparam(&attr, &param);
//	assert(!err);
//	param.sched_priority += 10;
//	err = pthread_attr_setschedparam(&attr, &param);
	assert(!err);
	err = pthread_create(&mListenerThread, &attr, OSCListenerThread, this);
	assert(!err);
	
	// make outputs
	//
	for(int i=1; i <= kMLEngineMaxVoices * kNumOSCVoiceSignals; ++i)
	{
		if (!outputIsValid(i))
		{
			setOutput(i, getContext()->getNullOutput());
		}
	}

	// make voices
	//
	for(int i=0; i<kMLEngineMaxVoices; ++i)
	{
		ret = mVoices[i].resize(bufSize);		
		if (!ret == OK) break;
	}
	
	clear();
	
	return ret;
}

// it's uncommon for a processor to override getOutputIndex.  
// But unlike overriding getOutput, it's possible.
// we do it here because we have a variable number of outputs and would
// like to make names for them procedurally.
//
int MLProcOSCToSignals::getOutputIndex(const MLSymbol name) 
{ 
	// voice numbers are 1-indexed.
	int idx = 0;
	int voice = 0;
	int sig = 0;
	int len;
	const std::string nameStr = name.getString();
	const char* pName = nameStr.c_str();
	
	//debug() << "MLProcOSCToSignals::getOutputIndex: " << name << "\n";
	
	// match signal name with symbol text
	for(int n=0; n<kNumOSCVoiceSignals; ++n)
	{
		len = strlen(OSCVoiceSignalNames[n]);
		if (!strncmp(OSCVoiceSignalNames[n], pName, len))
		{
			sig = n + 1;
			break;
		}
	}
	
	// get voice number from end of symbol
	if (sig)
	{
		voice = name.getFinalNumber();
	}
	
	if (sig && voice)
	{
		if (voice <= mCurrentVoices)
		{
			idx = (voice - 1)*kNumOSCVoiceSignals + sig;
		}
	}
	
	if (!idx)
	{
		MLError() << "MLProcOSCToSignals::getOutputIndex: null output " << name << "\n";	
	}

    debug() << "MLProcOSCToSignals: output " << name << 	": " << idx << "\n";
	return idx; 
}

void MLProcOSCToSignals::setup()
{
	doParams();
}

void MLProcOSCToSignals::doParams()
{
	int newVoices = (int)getParam("voices");	
	
//	debug() << "MLProcOSCToSignals: " << newVoices << " voices.\n";

	if (newVoices != mCurrentVoices)
	{
		mCurrentVoices = newVoices;
		clear();
	}
		
	mParamsChanged = false;
//	dumpParams();	// DEBUG
	
}

MLProc::err MLProcOSCToSignals::prepareToProcess()
{
	clear();
	return OK;
}

void MLProcOSCToSignals::clear()
{
//	const ScopedLock l (getProcessLock()); 
	int bufSize = (int)getParam("bufsize");
	int vecSize = getContextVectorSize();
	
	debug() << "clearing MLProcOSCToSignals: bufsize" << bufSize << ", vecSize " << vecSize << "\n";
	
	//PaUtil_FlushRingBuffer( &mNoteBuf );

	int outs = getNumOutputs();
	if (outs)
	{
		for (int v=0; v<kMLEngineMaxVoices; ++v)
		{
			mVoices[v].clear();
		}
	}

}

// order of signals:
// pitch 
// amp (gate * velocity)
// vel (velocity, stays same after note off)
// voice 
// mod 
// aftertouch
//


// display MIDI: pitch vel voice after mod -2 -3 -4

// display OSC: pitch vel voice after(z) x y dx dy


void MLProcOSCToSignals::process(const int frames)
{	
	float x, y, z, sx, sy;
	int age;

//	const ScopedLock l (getProcessLock()); 
	
//	int vecSize = getContextVectorSize();
	int sr = getContextSampleRate();
	
	if (mParamsChanged) doParams();
			
	// TEMP get most recent frame and apply to whole buffer			
	// read from mFrameBuf, which is being filled up by OSC listener thread
	int avail = 0;
	int framesRead = 0;
	
	avail = PaUtil_GetRingBufferReadAvailable(&mFrameBuf);
	if (avail) do
	{
		framesRead = PaUtil_ReadRingBuffer(&mFrameBuf, &mLatestFrame, 1);
		if (!framesRead == 1)
		{
//			printf(stderr, "error: read from ring buffer returned %d\n", read);
		}
		avail = PaUtil_GetRingBufferReadAvailable(&mFrameBuf);
		mFrameCount++;
	}
	while (avail > 0);	
	
	
	// changes per voice
	for (int v=0; v<kMLEngineMaxVoices; ++v)
	{
		float f, pressure;
		
		MLSignal& pitchSig = getOutput(v*kNumOSCVoiceSignals + 1);
		MLSignal& ampSig = getOutput(v*kNumOSCVoiceSignals + 2);
		MLSignal& pressureSig = getOutput(v*kNumOSCVoiceSignals + 3);
		MLSignal& voiceSig = getOutput(v*kNumOSCVoiceSignals + 4);
		MLSignal& dxSig = getOutput(v*kNumOSCVoiceSignals + 5);
		MLSignal& dySig = getOutput(v*kNumOSCVoiceSignals + 6);
				
		if (v < mCurrentVoices)
		{
			x = mLatestFrame(0, v);
			y = mLatestFrame(1, v);
			z = mLatestFrame(2, v);
			age = mLatestFrame(3, v);
			
			if (age >= 1) 
//			if (z > 0.f)
			{
				// process note on
				if (mVoices[v].mZ1 <= 0.)
				{
					debug() << "N" << v << "! ";
					mVoices[v].mStartX = x;
					mVoices[v].mStartY = y;
				}
				else
				{
					// note continues
				}
				mVoices[v].mX1 = x;
				mVoices[v].mY1 = y;
			}
			else
			{
				// process note off
				mVoices[v].mStartX = x;
				mVoices[v].mStartY = y;
				x = mVoices[v].mX1;
				y = mVoices[v].mY1;
			}
			
			f = xyToPitch(x, y);
			pressure = zToPressure(z);
			
			sx = xToDx(x);
			sy = yToDy(y);

			pitchSig.setToConstant(f); 
			ampSig.setToConstant(pressure); 
			pressureSig.setToConstant(pressure); 
			dxSig.setToConstant(sx); 
			dySig.setToConstant(sy); 
		}
		else
		{
			pitchSig.setToConstant(0.f); 
			ampSig.setToConstant(0.f); 
			pressureSig.setToConstant(0.f); 
			dxSig.setToConstant(0.f); 
			dySig.setToConstant(0.f); 
		}
		
		voiceSig.setToConstant(v); 
		
		// z-1
		mVoices[v].mZ1 = z;

	}
	
	
	mTest += frames;
	if (mTest > sr) // temp
	{
		mTest -= sr;
		debug() << "tick. ";
		
		// audio callback CPU
		// double cpu = mDeviceManager.getCpuUsage();
		
		debug() << "*** got " << mFrameCount << " frames. \n";
		mFrameCount = 0;
		
		debug() << mCurrentVoices << " output channels, " << frames << "samples .\n";
		debug() << "latest frame: \n";

		for(int v=0; v<mCurrentVoices; ++v)
		{
			x = mLatestFrame(0, v);
			y = mLatestFrame(1, v);
			z = mLatestFrame(2, v);
			age = mLatestFrame(3, v);
			
			debug() << "    t" << v << " x:" << x << " y:" << y << " z:" << z << " [" << age << "]\n";
		}
		debug() << "\n";
//	dumpEvents();	// DEBUG
//	dumpVoices();	// DEBUG
	}
			
			
}
	
	
// MIDI note 0 is C-1. 
// MIDI note 9 is A-1, 13.75 Hz. 
// MIDI note 21 is A0, 27.5 Hz. 
// MIDI note 117 is A8,  Hz. 
// pitch is returned as an exponent e where 2^e = frequency.
// midiToPitch is a linear mapping, [21, 117] -> [-4, 4.]

const int kRows = 5;
//float rowPositions[kRows + 2] = {0.f, 0.1515f, 0.3636f, 0.6363f, 0.8484f, 1.f, 99.f};
float rowPositions[kRows + 2] = {0.f, 0.2f, 0.4f, 0.6f, 0.8f, 1.f, 99.f};

MLSample MLProcOSCToSignals::xyToPitch(float x, float y) // TODO load scales
{
	// quick and dirty!
	// get row 0-4
//	float my = 1.f - y;
	int row = 0;
	for(row=0; row<kRows; row++)
	{
		if(y < rowPositions[row+1]) 
		{
			break;
		}
	}
	
	// get col 0-30
	// cols are over carriers 2-61 
	int col = 0;
	float carrier = x*64.f;
	clamp(carrier, 2.f, 62.f);
	carrier = carrier/2.f - 1.f;
	col = carrier;
	
	// get MIDI note equiv
	int note = col + row*5;
	note = clamp(note, 1, 127);
	
	float freq = mNoteTable[note];
	return freq;
	
}

MLSample MLProcOSCToSignals::xToDx(float x) // TEMP
{
	// get col 0-30
	// cols are over carriers 2-61 
	int col = 0;
	float carrier = x*64.f;
	clamp(carrier, 2.f, 62.f);
	carrier = carrier/2.f - 1.f;
	col = carrier;
	
	float dx = (carrier - ((float)col) - 0.5) * 2.;
	
	return dx;
}

MLSample MLProcOSCToSignals::yToDy(float y) // TEMP
{
	// quick and dirty!
	// get row 0-4
//	float my = 1.f - y;
	int row = 0;
	for(row=0; row<kRows; row++)
	{
		if(y < rowPositions[row+1]) 
		{
			break;
		}
	}
	
	float rowLo = rowPositions[row];
	float rowHi = rowPositions[row+1];
	
	float py = ((y - rowLo) / (rowHi - rowLo)) - 0.5;
	py *= 2.f;
	return py;
}


MLSample MLProcOSCToSignals::zToPressure(float z)
{
	return z;
}

void MLProcOSCToSignals::dumpVoices()
{
	for (int i=0; i<kMLEngineMaxVoices; ++i)
	{
		MLOSCVoice& voice = mVoices[i];
//		debug() << " [" << voice.mNote << "]";
		if (voice.mActive)
			debug() << "*";
	}
	debug() << "\n";
}

//--------------------------------------------------------------------------------
#pragma mark OSCListenerThread

void *OSCListenerThread(void *arg)
{
	MLProcOSCToSignals* oscToSig = static_cast<MLProcOSCToSignals*>(arg);
	PaUtilRingBuffer* pBuffer = oscToSig->getFrameBuffer();
	
	SoundplaneOSCListener listener;
	listener.setOutputBuffer(pBuffer);
	UdpListeningReceiveSocket socket(
            IpEndpointName( IpEndpointName::ANY_ADDRESS, PORT ),
            &listener );
			
	socket.RunUntilSigInt();
	return NULL;
}


