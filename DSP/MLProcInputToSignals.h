
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_MIDI_TO_SIGNALS_H
#define ML_PROC_MIDI_TO_SIGNALS_H

#include "AppConfig.h"

#include "MLDSP.h"
#include "MLProc.h"
#include "MLScale.h"
#include "MLChangeList.h"
#include "MLInputProtocols.h"
#include "pa_ringbuffer.h"

#include <stdexcept>

// states to mark an event's connection to one or more voices. 
// states > 0 mean voices are active.
const long kVoiceOff = -1;
const long kVoicePending = -2;
const long kVoiceUnison = 1<<14;

const int kMLMaxEvents = 1 << 4;
const int kMLEventMask = kMLMaxEvents - 1;
const int kNoteBufElements = 512;

const float kControllerScale = 1.f/127.f;
const float kDriftConstantsAmount = 0.004f;
const float kDriftRandomAmount = 0.002f;

const int kOSCToSignalsFrameWidth = 4;
const int kOSCToSignalsFrameHeight = 16;
const int kOSCToSignalsFrameBufferSize = 128;

// a key that is down.
class MLKeyEvent 
{
public:
	MLKeyEvent();
	~MLKeyEvent() {};
	void clear();
	void setup(int note, int vel, int time, int order);
	void setVoice(int v);
	bool isSounding();
	
	int mNote;
	int mVel;
	int mStartTime;
	int mVoiceState;	// zero or positive for voice we are assigned to, or negative for status flags
	int mStartOrder;	// always increasing from event to event
};

// a voice that can play. 
class MLVoice
{
public:
	MLVoice();
	~MLVoice() {} ;
	void clearState();
	void clearChanges();
	MLProc::err resize(int size);
	void zero();
	
	int mActive;
	int mNote;
	
	// for continuous touch inputs (Soundplane / OSC)
	float mStartX;
	float mStartY;
	float mStartPitch;
	float mPitch;
	float mX1;
	float mY1;
	float mZ1;

	MLChangeList mdPitch;
	MLChangeList mdAmp;
	MLChangeList mdVel;
	MLChangeList mdAfter;
	MLChangeList mdMod;
	MLChangeList mdMod2;
	MLChangeList mdMod3;
	MLChangeList mdDrift;
};

extern const int kNumVoiceSignals;
extern const char * voiceSignalNames[];

class MLProcInputToSignals : public MLProc
{
public:

	 MLProcInputToSignals();
	~MLProcInputToSignals();

	void setInputFrameBuffer(PaUtilRingBuffer* pBuf);
	void clear();
	MLProc::err prepareToProcess();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
	void clearMIDI();
 	void setup();
 	err resize();
	void setMIDIFrameOffset(int offset);
	int getOutputIndex(const MLSymbol name);
	
	// events
	int findEventForNote(int note);
	void clearEvent(MLKeyEvent& event, int time);

	void addNoteOn(int note, int vel, int time);
	void addNoteOff(int note, int vel, int time);
	void allNotesOff();
	void setController(int controller, int value, int time);
	void setPitchWheel(int value, int time);
	void setAfterTouch(int note, int value, int time);
	void setChannelAfterTouch(int value, int time);
	void setSustainPedal(int value, int time);
	
	MLScale* getScale();
	MLSample noteToPitch(float note);
	MLSample midiToPitch(int note);
	MLSample velToAmp(int vel);

	void doParams();
	
	// any code that needs to run while processing is not occurring must get this lock.
//	const CriticalSection& getProcessLock() const noexcept { return mProcessLock; }

private:
	MLProcInfo<MLProcInputToSignals> mInfo;
	
	int mProtocol;
	
//	CriticalSection mProcessLock;
	PaUtilRingBuffer* mpFrameBuf;
	MLSignal mLatestFrame;
	
	void dumpEvents();
	void dumpVoices();
	
	int allocate();
	void sendEventToVoice(MLKeyEvent& e, int voiceIdx);

	void doNoteOn(int note, int vel, int time);
	void doNoteOff(int note, int time);
	
	MLKeyEvent mEvents[kMLMaxEvents];
	int mNextEventIdx;
	
	MLVoice mVoices[kMLEngineMaxVoices]; 
	int mNextVoiceIdx;
	
	PaUtilRingBuffer mNoteBuf;
	int mNoteBufData[kNoteBufElements];
		
	MLChangeList mdChannelAfterTouch;
	MLChangeList mdPitchBend;
	MLChangeList mdController;
	MLChangeList mdController2;
	MLChangeList mdController3;
	int mControllerNumber;
	
	int mMIDIFrameOffset;
	MLRange mPitchRange;
	MLRange mAmpRange;
	bool mRetrig;
	bool mUnisonMode;
	bool mDriftMode;
	float mGlide;	

	int mCurrentVoices;
	int mDriftCounter;
	int mEventCounter;
	
	MLSignal mPitchBendSignal;
	MLSignal mDriftSignal;
	MLSignal mChannelAfterTouchSignal;
	MLSignal mControllerSignal;
	MLSignal mControllerSignal2;
	MLSignal mControllerSignal3;

	float mPitchWheelSemitones;
	MLScale mScale;
	
	int temp;

MLSample xyToPitch(float x, float y);
	float mNoteTable[128]; // TEMP

};


#endif // ML_PROC_MIDI_TO_SIGNALS_H