
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

// a key that is down.
class MLKeyEvent 
{
public:
	// states to mark an event's connection to one or more voices. 
	// states > 0 mean voices are active.
	static const long kVoiceOff = -1;
	static const long kVoicePending = -2;
	static const long kVoiceUnison = 1<<14;

	MLKeyEvent();
	~MLKeyEvent() {};
	void clear();
	void setup(int note, int vel, int time, int order);
	void setVoice(int v);
	inline bool isSounding() { return (mVoiceState >= 0); }
	
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
	int mAge;	// time in this state, measured to the end of the current process buffer
	
	// for continuous touch inputs (Soundplane / OSC)
	float mStartX;
	float mStartY;
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

const int kMLMaxEvents = 1 << 4;
const int kMLEventMask = kMLMaxEvents - 1;
const int kNoteBufElements = 512;

class MLProcInputToSignals : public MLProc
{
public:

	static const float kControllerScale;
	static const float kDriftConstantsAmount;
	static const float kDriftRandomAmount;

	static const int kFrameWidth = 4;
	static const int kFrameHeight = 16;
	static const int kFrameBufferSize = 128;

	 MLProcInputToSignals();
	~MLProcInputToSignals();

	void setInputFrameBuffer(PaUtilRingBuffer* pBuf);
	void clear();
	MLProc::err prepareToProcess();
	void process(const int n);		
	void processOSC(const int n);		
	void processMIDI(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
	void clearMIDI();
 	void setup();
 	err resize();
	void setMIDIFrameOffset(int offset);
	int getOutputIndex(const MLSymbol name);
	
	// events
	int findEventForNote(int note);
	void clearEvent(MLKeyEvent& event, int time);
	bool hasHeldKeyEvent(int v);

	void addNoteOn(int note, int vel, int time);
	void addNoteOff(int note, int vel, int time);
	void allNotesOff();
	void setRetrig(bool r);
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

private:
	MLProcInfo<MLProcInputToSignals> mInfo;
	
	int mProtocol;
	
	PaUtilRingBuffer* mpFrameBuf;
	MLSignal mLatestFrame;
	
	void dumpEvents();
	void dumpVoices();
	
	int allocate();
	void sendEventToVoice(MLKeyEvent& e, int voiceIdx, int frames);

	void doNoteOn(int note, int vel, int time, int frames);
	void doNoteOff(int note, int time, int frames);
	
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
	int mUnisonInputTouch;
	bool mDriftMode;
	float mGlide;	
	int mOSCDataRate;
	
	float mUnisonPitch1;

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
	bool mSustain;
};


#endif // ML_PROC_MIDI_TO_SIGNALS_H