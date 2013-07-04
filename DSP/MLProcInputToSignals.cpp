
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcInputToSignals.h"
 
const float kDriftConstants[16] = 
{
	0.465f, 0.005f, 0.013f, 0.019f, 
	0.155f, 0.933f, 0.002f, 0.024f,
	0.943f, 0.924f, 0.139f, 0.501f,
	0.196f, 0.591f, 0.961f, 0.442f
};

const int kNumVoiceSignals = 9;
const char * voiceSignalNames[kNumVoiceSignals] = 
{
	"pitch",
	"amp",
	"vel",
	"voice",
	"after",
	"moda",
	"modb",
	"modc",
	"position"
};

const float MLProcInputToSignals::kControllerScale = 1.f/127.f;
const float MLProcInputToSignals::kDriftConstantsAmount = 0.004f;
const float MLProcInputToSignals::kDriftRandomAmount = 0.002f;

const int MLProcInputToSignals::kFrameWidth = 4;
const int MLProcInputToSignals::kFrameHeight = 16;
const int MLProcInputToSignals::kFrameBufferSize = 128;

// ----------------------------------------------------------------
//
#pragma mark MLKeyEvent
// 

MLKeyEvent::MLKeyEvent()
{
	clear();
}

void MLKeyEvent::clear()
{
	mNote = 0;
	mVel = 0;
	mStartTime = 0;
	mVoiceState = kVoiceOff;
}

void MLKeyEvent::setup(int note, int vel, int time, int count)
{
	mNote = note;
	mVel = vel;
	mStartTime = time;
	mStartOrder = count;
}

void MLKeyEvent::setVoice(int v)
{
	mVoiceState = v;
}

// ----------------------------------------------------------------
//
#pragma mark MLVoice
// 

static const int kDriftInterval = 10;

MLVoice::MLVoice() : 
	mStartX(0.),
	mStartY(0.),
	mPitch(0.),
	mX1(0.),
	mY1(0.),
	mZ1(0.),
	mAge(0)
{
	mActive = 0;
	mNote = 0;
	clearState();
	clearChanges();
}

MLProc::err MLVoice::resize(int bufSize)
{
	MLProc::err ret = MLProc::OK;

	// make delta lists
	// allow for one change each sample, though this is unlikely to get used.
	MLProc::err a = mdPitch.setDims(bufSize);	
	MLProc::err b = mdAmp.setDims(bufSize);
	MLProc::err c = mdVel.setDims(bufSize);
	MLProc::err d = mdAfter.setDims(bufSize);
	MLProc::err e = mdMod.setDims(bufSize);
	MLProc::err f = mdMod2.setDims(bufSize);
	MLProc::err g = mdMod3.setDims(bufSize);
	MLProc::err h = mdDrift.setDims(bufSize);
	
	if (a || b || c || d || e || f || g || h)
	{
		ret = MLProc::memErr;
	}
	
	return ret;
}

void MLVoice::clearState()
{
	mActive = 0;
	mNote = 0;
	mAge = 0;
}

// clear changes but not current state.
void MLVoice::clearChanges()
{
	mdPitch.clearChanges();
	mdAmp.clearChanges();
	mdVel.clearChanges();
	mdAfter.clearChanges();
	mdMod.clearChanges();
	mdMod2.clearChanges();
	mdMod2.clearChanges();
}

void MLVoice::zero()
{
	mdPitch.zero();
	mdAmp.zero();
	mdVel.zero();
	mdAfter.zero();
	mdMod.zero();
	mdMod2.zero();
	mdMod3.zero();
}

// ----------------------------------------------------------------
//
#pragma mark MLProcInputToSignals
// 

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcInputToSignals> classReg("midi_to_signals");
	ML_UNUSED MLProcParam<MLProcInputToSignals> params[8] = { "bufsize", "voices", "bend", "mod", "unison", "glide", "protocol", "data_rate" };
	// no input signals.
	ML_UNUSED MLProcOutput<MLProcInputToSignals> outputs[] = {"*"};	// variable outputs
}	

MLProcInputToSignals::MLProcInputToSignals() :
	mCurrentVoices(0),
	mProtocol(-1),
	mpFrameBuf(0),
	mUnisonInputTouch(-1),
	mSustain(false)
{
//	debug() << "MLProcInputToSignals constructor:\n";
	setParam("voices", 0);	// default
	setParam("protocol", kInputProtocolMIDI);	// default
	setParam("data_rate", 100);	// default
	mMIDIFrameOffset = 0;
	
	// setup midi vel to amp
	mAmpRange.set(1, 127);
	mAmpRange.convertTo(MLRange(0.1f, 1.f));	// to be squared

	mNextVoiceIdx = 0;
	mNextEventIdx = 0;
	mRetrig = true;	// not sure of alg for velocity otherwise
	
	mPitchWheelSemitones = 7.f;
	mdPitchBend.clearChanges();
	
	mUnisonMode = false;
	mEventCounter = 0;
	mDriftCounter = -1;
	
	temp = 0;
	mOSCDataRate = 100;
	
	// ring buffer for incoming note events
	PaUtil_InitializeRingBuffer( &mNoteBuf, 4, kNoteBufElements, mNoteBufData );	
}

MLProcInputToSignals::~MLProcInputToSignals()
{
//	debug() << "MLProcInputToSignals destructor\n";
}

// set frame buffer for OSC inputs
void MLProcInputToSignals::setInputFrameBuffer(PaUtilRingBuffer* pBuf)
{
	mpFrameBuf = pBuf;
}

// executed for evey incoming MIDI buffer!
void MLProcInputToSignals::clearMIDI() 
{
	// global things
	mdPitchBend.clearChanges();
	mdController.clearChanges();
	mdController2.clearChanges();
	mdController3.clearChanges();
	mdChannelAfterTouch.clearChanges();
	
	// things per voice
	for (int v=0; v<kMLEngineMaxVoices; ++v)
	{
		mVoices[v].clearChanges();
	}
}

// set up output buffers
MLProc::err MLProcInputToSignals::resize() 
{
	MLProc::err re = OK;
	if (mParamsChanged) doParams();
	int bufSize = (int)getParam("bufsize");
	int vecSize = getContextVectorSize();
	MLSampleRate rate = getContextSampleRate();
	
	// make signals that apply to all voices
	mPitchBendSignal.setDims(vecSize);
	mDriftSignal.setDims(vecSize);
	mChannelAfterTouchSignal.setDims(vecSize);
	mControllerSignal.setDims(vecSize);
	mControllerSignal2.setDims(vecSize);
	mControllerSignal3.setDims(vecSize);

	// setup global change lists
	const float glideTime = 0.01f;
	mdChannelAfterTouch.setDims(bufSize);
	mdChannelAfterTouch.setSampleRate(rate);
	mdChannelAfterTouch.setGlideTime(glideTime);

	mdPitchBend.setDims(bufSize);
	mdPitchBend.setSampleRate(rate);
	mdPitchBend.setGlideTime(glideTime);

	mdController.setDims(bufSize);
	mdController.setSampleRate(rate);
	mdController.setGlideTime(glideTime);
	
	mdController2.setDims(bufSize);
	mdController2.setSampleRate(rate);
	mdController2.setGlideTime(glideTime);
	
	mdController3.setDims(bufSize);
	mdController3.setSampleRate(rate);
	mdController3.setGlideTime(glideTime);
	
	if (!mLatestFrame.setDims(kFrameWidth, kFrameHeight))
	{
		return MLProc::memErr;
	}
	
	// make outputs
	//
	for(int i=1; i <= kMLEngineMaxVoices * kNumVoiceSignals; ++i)
	{
		if (!outputIsValid(i))
		{
			setOutput(i, getContext()->getNullOutput());
		}
	}

	// make voices
	//
	MLProc::err r;
	for(int i=0; i<kMLEngineMaxVoices; ++i)
	{
		r = mVoices[i].resize(bufSize);
		if (!r == OK) break;
		
		// set initial pitch to 0.
		mVoices[i].mdPitch.addChange(0.f, 1);
		MLSignal& out = getOutput(i*kNumVoiceSignals + 1);
		mVoices[i].mdPitch.writeToSignal(out, 0, vecSize);
		mVoices[i].mdDrift.setGlideTime(kDriftInterval);	
	}
	
	// clear change lists
//	setMIDIFrameOffset(0);
	clearMIDI();
	
	return re;
}

// it's uncommon for a processor to override MLProc::getOutputIndex.  
// But unlike overriding MLProc::getOutput, it's possible.
// we do it here because we have a variable number of outputs and would
// like to make names for them procedurally.
int MLProcInputToSignals::getOutputIndex(const MLSymbol name) 
{ 
	// voice numbers are 1-indexed.
	int idx = 0;
	int voice = 0;
	int sig = 0;
	int len;
	const std::string nameStr = name.getString();
	const char* pName = nameStr.c_str();
		
	// match signal name with symbol text
	for(int n=0; n<kNumVoiceSignals; ++n)
	{
		len = strlen(voiceSignalNames[n]);
		if (!strncmp(voiceSignalNames[n], pName, len))
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
			idx = (voice - 1)*kNumVoiceSignals + sig;
		}
	}
	
	if (!idx)
	{
		MLError() << "MLProcInputToSignals::getOutputIndex: null output " << name << "\n";	
	}

	// debug() << "MLProcInputToSignals:getOutputIndex output " << name << 	": " << idx << "\n";
	return idx; 
}

// set offset from start of sample buffer to start of MIDI buffer.
void MLProcInputToSignals::setMIDIFrameOffset(int offset)
{
	mMIDIFrameOffset = offset;
}

void MLProcInputToSignals::setup()
{
	doParams();
}

void MLProcInputToSignals::doParams()
{
	int newVoices = (int)getParam("voices");	
// debug() << "MLProcInputToSignals: " << newVoices << " voices.\n";
// TODO enable / disable voice containers here
	mOSCDataRate = (int)getParam("data_rate");

	int newProtocol = (int)getParam("protocol");	
	if (newProtocol != mProtocol)
	{
		mProtocol = newProtocol;
	}
	
	switch(mProtocol)
	{
		case kInputProtocolOSC:	
			for(int i=0; i<kMLEngineMaxVoices; ++i)
			{				
				// TODO fix names, amp and vel are really switched. 
				// amp snaps to new velocity right away 
				mVoices[i].mdAmp.setGlideTime(0.0f);
				mVoices[i].mdVel.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdAfter.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdMod.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdMod2.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdMod3.setGlideTime(1.f / (float)mOSCDataRate);
			}

			break;
		case kInputProtocolMIDI:	
			for(int i=0; i<kMLEngineMaxVoices; ++i)
			{
				mVoices[i].mdAmp.setGlideTime(0.0f);
			}
			break;
	}
	
	if (newVoices != mCurrentVoices)
	{
		mCurrentVoices = newVoices;
		allNotesOff();
	}
	
	// pitch wheel mult
	mPitchWheelSemitones = getParam("bend");
	
	// listen to controller number mod
	int num = (int)getParam("mod");
	if (mControllerNumber != num)
	{
		mdController.clearChanges();
		mdController2.clearChanges();
		mdController3.clearChanges();
		mControllerNumber = num;
	}
	
	int unison = (int)getParam("unison");
	if (mUnisonMode != unison)
	{
		mUnisonMode = unison;
		allNotesOff();
	}
	
	mGlide = getParam("glide");
	for (int v=0; v<kMLEngineMaxVoices; ++v)
	{
		mVoices[v].mdPitch.setGlideTime(mGlide);
	}
		
	mParamsChanged = false;
//dumpParams();	// DEBUG
}

inline int packNote(int note, int vel, int time)
{
	int r = 0;
	r |= note & 0xFF;
	r |= (vel & 0xFF) << 8;
	r |= (time & 0xFFFF) << 16;
	return r;
}

inline void unpackNote(int packed, int& note, int& vel, int& time)
{
	note = packed & 0xFF;
	vel = (packed & 0xFF00) >> 8;
	time = (packed & 0xFFFF0000) >> 16;
}

MLProc::err MLProcInputToSignals::prepareToProcess()
{
	allNotesOff();
	setMIDIFrameOffset(0);
	return OK;
}

void MLProcInputToSignals::clear()
{
	int vecSize = getContextVectorSize();
	
	//debug() << "clearing MLProcInputToSignals: bufsize" << bufSize << ", vecSize " << vecSize << "\n";
	clearMIDI();
	
	PaUtil_FlushRingBuffer( &mNoteBuf );
	for(int i=0; i<kMLMaxEvents; ++i)
	{
		mEvents[i].clear();
	}

	int outs = getNumOutputs();
	if (outs)
	{
		for (int v=0; v<kMLEngineMaxVoices; ++v)
		{
			mVoices[v].clearState();
			mVoices[v].clearChanges();
			mVoices[v].zero();
			
			mVoices[v].mdPitch.writeToSignal(getOutput(v*kNumVoiceSignals + 1), 0, vecSize);
			mVoices[v].mdAmp.writeToSignal(getOutput(v*kNumVoiceSignals + 2), 0, vecSize);
			mVoices[v].mdVel.writeToSignal(getOutput(v*kNumVoiceSignals + 3), 0, vecSize); 
			getOutput(v*kNumVoiceSignals + 4).setToConstant(v); 
			mVoices[v].mdAfter.writeToSignal(getOutput(v*kNumVoiceSignals + 5), 0, vecSize);
			mVoices[v].mdMod.writeToSignal(getOutput(v*kNumVoiceSignals + 6), 0, vecSize);
			mVoices[v].mdMod2.writeToSignal(getOutput(v*kNumVoiceSignals + 7), 0, vecSize);
			mVoices[v].mdMod3.writeToSignal(getOutput(v*kNumVoiceSignals + 8), 0, vecSize);
		}
	}
	mEventCounter = 0;
}

// order of signals:
// pitch 
// amp (gate * velocity)
// vel (velocity, stays same after note off)
// voice 
// aftertouch
// mod, mod2, mod3
//

// ????


const int kRows = 5;
//float rowPositionsMTS[kRows + 2] = {0.f, 0.1515f, 0.3636f, 0.6363f, 0.8484f, 1.f, 99.f};
float rowPositionsMTS[kRows + 2] = {0.f, 0.2f, 0.4f, 0.6f, 0.8f, 1.f, 99.f};

/*
MLSample xToDx(float x);
MLSample xToDx(float x) // TEMP
{
	// get col 0-30
	// cols are over carriers 2-61 
	int col = 0;
	float carrier = x*64.f;
	clamp(carrier, 2.f, 62.f);
	col = (carrier - 1.5f)/2.f;
	
	
	float dx = (carrier - ((float)col) - 0.5) * 2.;
	
	return dx;
}
*/

/*
MLSample yToDy(float y);
MLSample yToDy(float y) // TEMP
{
	// quick and dirty!
	// get row 0 - 4
	int row = 0;
	for(row=0; row<kRows; row++)
	{
		if(y + 0.6f < rowPositionsMTS[row+1]) 
		{
			break;
		}
	}
	
	float rowLo = rowPositionsMTS[row];
	float rowHi = rowPositionsMTS[row+1];
	
	float py = ((y - rowLo) / (rowHi - rowLo)) - 0.5;
	py *= 2.f;
	return py;
}
*/

// display MIDI: pitch vel voice after mod -2 -3 -4

// display OSC: pitch vel(constant during hold) voice(touch) after(z) dx dy x y

// TODO sustain

#pragma mark -

void MLProcInputToSignals::process(const int frames)
{	
	int sr = getContextSampleRate();	 
	if (mParamsChanged) doParams();

	// update drift change list for each voice
	if ((mDriftCounter < 0) || (mDriftCounter > sr*kDriftInterval))
	{
		for (int v=0; v<mCurrentVoices; ++v)
		{
			float drift = (kDriftConstants[v] * kDriftConstantsAmount) + (MLRand()*kDriftRandomAmount);
			mVoices[v].mdDrift.addChange(drift, 1);
		}		
		mDriftCounter = 0;
	}	
	mDriftCounter += frames;
				
	// update age for each voice
	for (int v=0; v<mCurrentVoices; ++v)
	{
		if(mVoices[v].mAge >= 0)
		{
			mVoices[v].mAge += frames;
		}
	}		

	// make voice number signal for each voice
	for (int v=0; v<kMLEngineMaxVoices; ++v)
	{
		MLSignal& vox = getOutput(v*kNumVoiceSignals + 4);
		if (v < mCurrentVoices)
		{
			vox.setToConstant(v); 
		}
		else
		{
			vox.setToConstant(0.f); 
		}
	}

	switch(mProtocol)
	{
		case kInputProtocolOSC:	
			processOSC(frames);
			break;
		case kInputProtocolMIDI:	
			processMIDI(frames);
			break;
	}
}

void MLProcInputToSignals::processOSC(const int frames)
{	
	float x, y, z, note;
	float dx, dy;
	int avail = 0;
	int framesRead = 0;
	
	// TODO this code only updates every signal vector.  Add sample-accurate
	// reading from OSC.

	// TEMP get most recent frame and apply to whole buffer			
	// read from mpFrameBuf, which is being filled up by OSC listener thread
	if(mpFrameBuf)
	{
		avail = PaUtil_GetRingBufferReadAvailable(mpFrameBuf);
		if (avail) do
		{
			framesRead = PaUtil_ReadRingBuffer(mpFrameBuf, mLatestFrame.getBuffer(), 1);
			if (!framesRead == 1)
			{
	//			printf(stderr, "error: read from ring buffer returned %d\n", read);
			}
			avail = PaUtil_GetRingBufferReadAvailable(mpFrameBuf);
		}
		while (avail > 0);	
	}

	// First turn touch frames into change lists, either in unison mode or not.
	if (mUnisonMode)
	{		
		// unison mode:
		// on any note-on for voice v, set mUnisonInputTouch to v and all voices to track voice v.
		// if triggerVoice = 0, retrigger envelopes.
		// on a note-off,
		// if v = mUnisonInputTouch, turn off all voices. 
		float ux = 0.;
		float uy = 0.;
		float uz = 0.;
		float upitch = mUnisonPitch1;
		float udx = 0.;
		float udy = 0.;
		
		for (int v=0; v<mCurrentVoices; ++v)
		{			
			x = mLatestFrame(0, v);
			y = mLatestFrame(1, v);
			z = mLatestFrame(2, v);
			note = mLatestFrame(3, v);
			dx = 0.;
			dy = 0.;
			if (z > 0.f)
			{
				if (mVoices[v].mZ1 <= 0.)
				{
					// turn unison voices on or change unison touch to newest
					mUnisonInputTouch = v;
					ux = mVoices[v].mStartX = x;
					uy = mVoices[v].mStartY = y;
					upitch = mVoices[v].mPitch = noteToPitch(note);
					udx = 0.f;
					udy = 0.f;		
				}
			}
			mVoices[v].mZ1 = z;
		}
		
		// update unison input touch.
		if(mUnisonInputTouch >= 0)
		{
			uz = mLatestFrame(2, mUnisonInputTouch);
			
			// if touch is removed, fall back to touch with maximum z
			if(uz <= 0.f)
			{
				// turn unison touch off
				mUnisonInputTouch = -1;

				float maxZ = 0;
				for (int v=0; v<mCurrentVoices; ++v)
				{
					float zz = mLatestFrame(2, v);
					if(zz > maxZ)
					{
						maxZ = zz;
						// found a fallback touch 
						mUnisonInputTouch = v;
					}
				}									
			}
			
			if(mUnisonInputTouch >= 0)
			{
				// unison continues				
				ux = mLatestFrame(0, mUnisonInputTouch);
				uy = mLatestFrame(1, mUnisonInputTouch);
				note = mLatestFrame(3, mUnisonInputTouch);
				upitch = noteToPitch(note);
				udx = ux - mVoices[mUnisonInputTouch].mStartX;
				udy = uy - mVoices[mUnisonInputTouch].mStartY;
			}
		}
		
		for (int v=0; v<mCurrentVoices; ++v)
		{			
			const int frameTime = 1;
			mVoices[v].mdPitch.addChange(upitch, frameTime);
			mVoices[v].mdAmp.addChange(uz, frameTime);
			mVoices[v].mdVel.addChange(uz, frameTime);
			
			mVoices[v].mdAfter.addChange(udx, frameTime);	
			mVoices[v].mdMod.addChange(udy, frameTime);
			mVoices[v].mdMod2.addChange(ux*2.f - 1.f, frameTime);
			mVoices[v].mdMod3.addChange(uy*2.f - 1.f, frameTime);		
		}	
		
		mUnisonPitch1 = upitch;	
	}
	else 
	{
		for (int v=0; v<mCurrentVoices; ++v)
		{
			x = mLatestFrame(0, v);
			y = mLatestFrame(1, v);
			z = mLatestFrame(2, v);
			note = mLatestFrame(3, v);
			dx = 0.;
			dy = 0.;
			
			if (z > 0.f)
			{
				if (mVoices[v].mZ1 <= 0.)
				{
					// process note on
					mVoices[v].mStartX = x;
					mVoices[v].mStartY = y;
					mVoices[v].mPitch = noteToPitch(note);
					dx = 0.f;
					dy = 0.f;
				}
				else
				{
					// note continues
					mVoices[v].mPitch = noteToPitch(note);
					dx = x - mVoices[v].mStartX;
					dy = y - mVoices[v].mStartY;
				}
				mVoices[v].mX1 = x;
				mVoices[v].mY1 = y;
			}
			else
			{
				if (mVoices[v].mZ1 > 0.)
				{
					// process note off, set pitch for release
					mVoices[v].mPitch = noteToPitch(note);
					x = mVoices[v].mX1;
					y = mVoices[v].mY1;
				}
			}

			mVoices[v].mZ1 = z;

			// OSC: pitch vel(constant during hold) voice(touch) after(z) dx dy x y			
			const int frameTime = 1;
			mVoices[v].mdPitch.addChange(mVoices[v].mPitch, frameTime);
			mVoices[v].mdAmp.addChange(z, frameTime);
			mVoices[v].mdVel.addChange(z, frameTime);
			
			mVoices[v].mdAfter.addChange(dx, frameTime);	
			mVoices[v].mdMod.addChange(dy, frameTime);
			mVoices[v].mdMod2.addChange(x*2.f - 1.f, frameTime);
			mVoices[v].mdMod3.addChange(y*2.f - 1.f, frameTime);		
		}	
	}	
	
	// write change lists out to signals.
	for (int v=0; v<kMLEngineMaxVoices; ++v)
	{
		// changes per voice
		MLSignal& pitch = getOutput(v*kNumVoiceSignals + 1);
		MLSignal& amp = getOutput(v*kNumVoiceSignals + 2);
		MLSignal& vel = getOutput(v*kNumVoiceSignals + 3);
		
		MLSignal& after = getOutput(v*kNumVoiceSignals + 5);
		MLSignal& mod = getOutput(v*kNumVoiceSignals + 6);
		MLSignal& mod2 = getOutput(v*kNumVoiceSignals + 7);
		MLSignal& mod3 = getOutput(v*kNumVoiceSignals + 8);

			
		if (v < mCurrentVoices)
		{
			mVoices[v].mdPitch.writeToSignal(pitch, mMIDIFrameOffset, frames);		
			// write to common temp drift signal, we add one change manually so read offset is 0
			mVoices[v].mdDrift.writeToSignal(mDriftSignal, 0, frames);
			pitch.add(mDriftSignal); 

			mVoices[v].mdAmp.writeToSignal(amp, mMIDIFrameOffset, frames);
			mVoices[v].mdVel.writeToSignal(vel, mMIDIFrameOffset, frames); 
			mVoices[v].mdAfter.writeToSignal(after, mMIDIFrameOffset, frames); 
			mVoices[v].mdMod.writeToSignal(mod, mMIDIFrameOffset, frames); 
			mVoices[v].mdMod2.writeToSignal(mod2, mMIDIFrameOffset, frames); 
			mVoices[v].mdMod3.writeToSignal(mod3, mMIDIFrameOffset, frames); 
		}
		else
		{
			pitch.setToConstant(0.f); 
			amp.setToConstant(0.f); 
			vel.setToConstant(0.f); 
			after.setToConstant(0.f); 
			mod.setToConstant(0.f); 
			mod2.setToConstant(0.f); 
			mod3.setToConstant(0.f); 
		}
	}
}

void MLProcInputToSignals::processMIDI(const int frames)
{	
	// pop note events from FIFO and create change lists for each voice
	int midiNote, vel, time;
	int unpacked;

	while(PaUtil_ReadRingBuffer( &mNoteBuf, &unpacked, 1 ))
	{
		unpackNote(unpacked, midiNote, vel, time);					
		if (time > frames - 1) time = frames - 1;
		if (vel)
		{
//	debug() << "+\n";
			doNoteOn(midiNote, vel, time, frames);

		}
		else
		{
//	debug() << "-\n";
			doNoteOff(midiNote, time, frames);
		}			
	}
	
	// write global change lists to signals -- same for all voices
	mdPitchBend.writeToSignal(mPitchBendSignal, mMIDIFrameOffset, frames);
	mdController.writeToSignal(mControllerSignal, mMIDIFrameOffset, frames);
	mdController2.writeToSignal(mControllerSignal2, mMIDIFrameOffset, frames);
	mdController3.writeToSignal(mControllerSignal3, mMIDIFrameOffset, frames);
	mdChannelAfterTouch.writeToSignal(mChannelAfterTouchSignal, mMIDIFrameOffset, frames);
	
	for (int v=0; v<kMLEngineMaxVoices; ++v)
	{
		// changes per voice
		MLSignal& pitch = getOutput(v*kNumVoiceSignals + 1);
		MLSignal& amp = getOutput(v*kNumVoiceSignals + 2);
		MLSignal& velSig = getOutput(v*kNumVoiceSignals + 3);
		MLSignal& after = getOutput(v*kNumVoiceSignals + 5);
		MLSignal& mod = getOutput(v*kNumVoiceSignals + 6);
		MLSignal& mod2 = getOutput(v*kNumVoiceSignals + 7);
		MLSignal& mod3 = getOutput(v*kNumVoiceSignals + 8);

		if (v < mCurrentVoices)
		{
			mVoices[v].mdPitch.writeToSignal(pitch, mMIDIFrameOffset, frames);			
			pitch.add(mPitchBendSignal); 		
			// write to common temp drift signal, we add one change manually so read offset is 0
			mVoices[v].mdDrift.writeToSignal(mDriftSignal, 0, frames);
			pitch.add(mDriftSignal); 
					
			mVoices[v].mdAmp.writeToSignal(amp, mMIDIFrameOffset, frames);
			
			mVoices[v].mdVel.writeToSignal(velSig, mMIDIFrameOffset, frames); 

			// aftertouch for each voice is channel aftertouch + poly aftertouch.
			mVoices[v].mdAfter.writeToSignal(after, mMIDIFrameOffset, frames);
			after.add(mChannelAfterTouchSignal); 				
			
			mod.clear();
			mod.add(mControllerSignal);
			
			mod2.clear();
			mod2.add(mControllerSignal2);
			
			mod3.clear();
			mod3.add(mControllerSignal3);
				
			mVoices[v].mdPitch.clearChanges();
			mVoices[v].mdDrift.clearChanges();
			mVoices[v].mdAmp.clearChanges();
			mVoices[v].mdAfter.clearChanges();
			mVoices[v].mdVel.clearChanges();
			mVoices[v].mdMod.clearChanges();
			mVoices[v].mdMod2.clearChanges();
			mVoices[v].mdMod3.clearChanges();
		}
		else
		{
			pitch.setToConstant(0.f); 
			amp.setToConstant(0.f); 
			velSig.setToConstant(0.f); 
			after.setToConstant(0.f); 
			mod.setToConstant(0.f); 
			mod2.setToConstant(0.f); 
			mod3.setToConstant(0.f); 
		}
	}
	
	/*
	// DEBUG
	MLSignal& amp = getOutput(2);
	bool up = false;
	for(int i=0; i<frames - 1; ++i)
	{
		float a = amp[i];
		float b = amp[i+1];
		if(b > a)
		{
			debug() << "up: amp[" << i << "] = " << a << "->" << "amp[" << i+1 << "] = " << b << "\n";		
			debug() << "amp is constant? " << amp.isConstant() << "\n";
			up = true;
			break;
		}
	}
		float c = amp[0];
		float d = amp[1];
		if( c+d > 12123123)
		{
			debug() << "\n";
		}
	if(up || anyOns)
	{
	debug() << "INPUT TO SIGNALS: " << anyOns << " ONS \n";
		amp.dump(true);
	}
	*/
}








			/*
			// test all constant
			pitch.setToConstant(pitch[0]); 
			amp.setToConstant(amp[0]); 
			vel.setToConstant(vel[0]); 
			vox.setToConstant(vox[0]); 
			mod.setToConstant(mod[0]);  
			after.setToConstant(after[0]); 
			*/
			
			/*
			// test none constant
			pitch.setConstant(false);
			amp.setConstant(false);
			vel.setConstant(false);
			vox.setConstant(false);
			mod.setConstant(false);
			after.setConstant(false);
			*/
			





MLScale* MLProcInputToSignals::getScale()
{
	return &mScale;
}
	
// MIDI note 0 is C-1. 
// MIDI note 9 is A-1, 13.75 Hz. 
// MIDI note 21 is A0, 27.5 Hz. 
// MIDI note 117 is A8,  Hz. 
// pitch is returned as an exponent e where 2^e = frequency.
// midiToPitch is a linear mapping, [21, 117] -> [-4, 4.]

MLSample MLProcInputToSignals::noteToPitch(float note)
{
	return MLSample(log2((float)mScale.noteToPitch(clamp(note, 0.f, 127.f))));
}

MLSample MLProcInputToSignals::midiToPitch(int note)
{
	return MLSample(log2((float)mScale.noteToPitch(clamp(note, 0, 127))));
}

// [0-127] -> [0.-1.]
MLSample MLProcInputToSignals::velToAmp(int vel) 
{
	MLSample amp;
	amp = mAmpRange(vel);
	amp *= amp;
	return amp;
}

// activate an event, taking over one voice.  translates note to pitch 
// and velocity to amplitude.
void MLProcInputToSignals::sendEventToVoice(MLKeyEvent& e, int voiceIdx, int bufFrames)
{
	int note = e.mNote;
	int vel = e.mVel;
	int time = e.mStartTime;

	e.mVoiceState = voiceIdx;
	if (voiceIdx == MLKeyEvent::kVoiceUnison)
	{
// debug() << "        unison vel = " << vel << "\n";
		for (int v=0; v<mCurrentVoices; ++v)
		{
			
			/*		
			// TODO if retrig is on, add voice off before voice on.
			if (mRetrig)
			{
				if (time == 0) time++; // in rare case where time = 0, make room for extra voice off.
				mVoices[v].mdAmp.addChange(0., time - 1);  
				mVoices[v].mdVel.addChange(0., time - 1);  
			}
			*/
			
			mVoices[v].mActive = true;
			mVoices[v].mNote = note;		
			mVoices[v].mAge = bufFrames - time;		
			mVoices[v].mdPitch.addChange(midiToPitch(note), time);
			mVoices[v].mdAmp.addChange(velToAmp(vel), time);
			mVoices[v].mdVel.addChange(velToAmp(vel), time);
		}
	}
	else if (voiceIdx >= 0)
	{
		if (mVoices[voiceIdx].mActive) // are we stealing?
		{
			/*
			// find event currently attached to this voice and clear it.
			for (int i=0; i<kMLMaxEvents; ++i)
			{
				int eventIdx = (mNextEventIdx - i) & kMLEventMask; // look backwards
				MLKeyEvent& event = mEvents[eventIdx];
				if (event.mVoiceState == voiceIdx)
				{
					event.clear();
					break;
				}
			}
			*/
			// if retrig is on, turn voice amp off before new note to force envelopes to retrigger.
			if (mRetrig)
			{
				if (time == 0) time++; // in case where time = 0, make room for turning amp off.
				mVoices[voiceIdx].mdAmp.addChange(0.f, time - 1);  
			}
		}		

		mVoices[voiceIdx].mActive = true;
		mVoices[voiceIdx].mNote = note;	
		mVoices[voiceIdx].mAge = bufFrames - time;		
		mVoices[voiceIdx].mdPitch.addChange(midiToPitch(note), time);
		mVoices[voiceIdx].mdAmp.addChange(velToAmp(vel), time);
		mVoices[voiceIdx].mdVel.addChange(velToAmp(vel), time);
	}
	/*
	// DEBUG
	debug() << "voices: ";
	for (int v=0; v<mCurrentVoices; ++v)
	{
		debug() << v << ":" << mVoices[v].mNote << " ";
	}
	debug() << "\n";
	*/
		
}

// return index of free voice or voice to steal. 
int MLProcInputToSignals::allocate()
{
	int r = 0;
	int found = false;
	
	// look for a free voice
	int n = mNextVoiceIdx;
	for (int v=0; v<mCurrentVoices; ++v)
	{
		n = (n + 1) % mCurrentVoices;
		if (!mVoices[n].mActive) 
		{
			r = mNextVoiceIdx = n;
			found = true;
			break;
		}
	}
	
	// look for any voice not matching a key held down
	// (possible with sustain pedal on)
	if(!found)
	{
		n = mNextVoiceIdx;
		for (int v=0; v<mCurrentVoices; ++v)
		{
			n = (n + 1) % mCurrentVoices;
			if (!hasHeldKeyEvent(n))
			{
				r = mNextVoiceIdx = n;
				found = true;
				break;
			}
		}
	}

	// if still not found, just steal oldest voice.
	if(!found)
	{
		int age;
		int maxAge = 0;
		int maxAgeIdx = 0;
		for (int v=0; v<mCurrentVoices; ++v)
		{
			age = mVoices[v].mAge;
			if (age > maxAge)
			{
				maxAge = age;
				maxAgeIdx = v;
			}
		}
		r = maxAgeIdx;
	}
//debug() << "allocate:  free voices " << freeVoices << "\n";

	return r;
}

void MLProcInputToSignals::dumpEvents()
{
	for (int i=0; i<kMLMaxEvents; ++i)
	{
		MLKeyEvent& event = mEvents[i];		
		debug() << " [" << event.mNote << "]";
		
		int state = event.mVoiceState;
		switch(state)
		{
		case MLKeyEvent::kVoiceOff:
			debug() << "-";
			break;		
		case MLKeyEvent::kVoicePending:
			debug() << "P";
			break;		
		case MLKeyEvent::kVoiceUnison:
			debug() << "U";
			break;		
		default:
			debug() << event.mVoiceState;
			break;		
			
		}
	}
	debug() << "\n";
}

void MLProcInputToSignals::dumpVoices()
{
	for (int i=0; i<kMLEngineMaxVoices; ++i)
	{
		MLVoice& voice = mVoices[i];
		debug() << " [" << voice.mNote << "]";
		if (voice.mActive)
			debug() << "*";
	}
	debug() << "\n";
}

// find event index matching held note  
int MLProcInputToSignals::findEventForNote(int note)
{
	int ret = -1;
	int i;
	for (i=0; i<kMLMaxEvents; ++i)
	{
//		int eventIdx = (mNextEventIdx - i) & kMLEventMask; // look backwards
		if (mEvents[i].mNote == note)
		{
//debug() << "found event [" << i << "] for note " << note << "\n";
			ret = i;
			break;
		}
	}	
	
	/*
	// TEST
	for (i++; i<kMLMaxEvents; ++i)
	{
		if (mEvents[i].mNote == note)
		{
//debug() << "ERROR:  found another event[" << i << "] for note " << note << "!! \n";
			clearEvent(mEvents[i], 1);
		}
	}	
	*/
	return ret;
}


// when crash here once: time == 1 , vel = 0
void MLProcInputToSignals::clearEvent(MLKeyEvent& event, int time)
{
	for (int v=0; v<mCurrentVoices; ++v)
	{
		if (mVoices[v].mNote == event.mNote)
		{
			mVoices[v].mActive = false;
			mVoices[v].mNote = 0;
			mVoices[v].mAge = 0;
			mVoices[v].mdAmp.addChange(0.f, time);
		}
	}
	event.clear();
	
	// DEBUG
	debug() << "clears: ";
	for (int v=0; v<mCurrentVoices; ++v)
	{
		debug() << v << ":" << mVoices[v].mNote << " ";
	}
	debug() << "\n";
		
}

void MLProcInputToSignals::addNoteOn(int note, int vel, int time)
{
	int n = packNote(note, vel, time);
	PaUtil_WriteRingBuffer( &mNoteBuf, &n, 1 );
}

void MLProcInputToSignals::addNoteOff(int note, int vel, int time)
{
	#pragma unused(vel)
	int n = packNote(note, 0, time);
	PaUtil_WriteRingBuffer( &mNoteBuf, &n, 1 );
}

void MLProcInputToSignals::doNoteOn(int note, int vel, int time, int frames)
{
	int newVoice;
		
	// get next free event
	int freeEventIdx = -1;
	for(int i=0; i<kMLMaxEvents; ++i)
	{
		mNextEventIdx = (mNextEventIdx + 1) & kMLEventMask;		
		if (mEvents[mNextEventIdx].mVoiceState == MLKeyEvent::kVoiceOff)
		{
			freeEventIdx = mNextEventIdx;
			break;
		}
	}
	
	if (freeEventIdx < 0)
	{
		debug() << "MLProcInputToSignals::doNoteOn: out of free events!\n";
		return; 
	}
				
	MLKeyEvent& newEvent = mEvents[freeEventIdx];
	newEvent.setup(note, vel, time, mEventCounter++);
	newEvent.mVoiceState = MLKeyEvent::kVoicePending;

	if (mUnisonMode)
	{		
		// mark all sounding events as pending.
		for (int i=0; i<kMLMaxEvents; ++i)
		{
			MLKeyEvent& event = mEvents[i];
			if (event.isSounding())
			{
// debug() << "marking note " << event.mNote << "as pending \n";			
				event.mVoiceState = MLKeyEvent::kVoicePending;
				break;
			}
		}
		sendEventToVoice(newEvent, MLKeyEvent::kVoiceUnison, frames);
	}
	else
	{
		newVoice = allocate();
		sendEventToVoice(newEvent, newVoice, frames);
	}
}

void MLProcInputToSignals::doNoteOff(int note, int time, int frames)
{
//debug() << "add note off " << note << " at time " << time << "\n";

	if (!mUnisonMode) // single voice per event
	{
		// could possibly activate stolen held notes here.  
		
		// clear key event
		for (int i=0; i<kMLMaxEvents; ++i)
		{
			if (mEvents[i].mNote == note)
			{
				mEvents[i].clear();
			}
		}	
		
		// if not sustaining, free voice
		if(!mSustain)
		{
			for (int v=0; v<mCurrentVoices; ++v)
			{
				if (mVoices[v].mNote == note)
				{
					mVoices[v].mActive = false;
					mVoices[v].mNote = 0;
					mVoices[v].mAge = 0;
					mVoices[v].mdAmp.addChange(0.f, time);					
				}
			}
		}
	}			
	else // unison
	{
		const int eventIdxToTurnOff = findEventForNote(note);
		if (eventIdxToTurnOff >= 0)
		{
			MLKeyEvent& event = mEvents[eventIdxToTurnOff];
			float holdVelocity = event.mVel;
			const bool eventWasSounding = (event.isSounding());
			mEvents[eventIdxToTurnOff].clear();
			if (eventWasSounding)
			{
				int orderAtIdx;
				int maxOrder = 0;
				int maxEventIdx = 0;
				bool eventPending = false;
			
				// get most recently created pending event and activate it,
				// but at just-erased event's velocity. 
				for (int i=0; i<kMLMaxEvents; ++i)
				{
					if (mEvents[i].mVoiceState == MLKeyEvent::kVoicePending) 
					{
						if (mEvents[i].mNote) //  is band-aid-- revisit algorithm
						{			
		//debug() << "event " << i << " has note " << mEvents[i].mNote << "  pending \n";
							eventPending = true;
							orderAtIdx = mEvents[i].mStartOrder;
							if (orderAtIdx > maxOrder)
							{
								maxEventIdx = i;
								maxOrder = orderAtIdx;
							}
						}
						else
						{
							mEvents[i].mVoiceState = MLKeyEvent::kVoiceOff;	
						}
					}
				}
			
				if (eventPending)
				{
	//debug() << " activating held note " << mEvents[maxEventIdx].mNote << " at time " << time << "\n";
					mEvents[maxEventIdx].mVel = (int)holdVelocity;
					mEvents[maxEventIdx].mStartTime = time;
					sendEventToVoice(mEvents[maxEventIdx], MLKeyEvent::kVoiceUnison, frames);
				}			
				else
				{
					// turn off all voices
					for (int i=0; i<mCurrentVoices; ++i)
					{
						mVoices[i].mdAmp.addChange(0.f, time);
						mVoices[i].mActive = false;
					}
				}
			}
		}
	}
}

void MLProcInputToSignals::allNotesOff()
{
	clear();
}

void MLProcInputToSignals::setRetrig(bool r)
{
	mRetrig = r;
}

void MLProcInputToSignals::setController(int controller, int value, int time)
{
	#pragma unused(time) // TODO
	float scaledVal = (float)value * kControllerScale;
	if (controller == mControllerNumber)
	{
		mdController.addChange(scaledVal, 1);
	}
	else if (controller == mControllerNumber + 1)
	{
		mdController2.addChange(scaledVal, 1);
	}
	else if (controller == mControllerNumber + 2)
	{
		mdController3.addChange(scaledVal, 1);
	}
}

void MLProcInputToSignals::setPitchWheel(int value, int time)
{
	#pragma unused(time) // TODO
	// set pitch multiplier for all voices.
	int zeroVal = value - 8192;
	float fval;
	if (zeroVal > 0)
	{
		fval = zeroVal / 8191.f;
	}
	else
	{
		fval = zeroVal / 8192.f;
	}
	
	float bendAdd = fval * mPitchWheelSemitones / 12.f;
	
//debug() << "pitch bend " << bendAdd << ", " << time << "\n";

//	mdPitchBend.addChange(bendAdd, time); 
	mdPitchBend.addChange(bendAdd, 1);	// 1 is a bandaid that is making it work in Live 8.1.5 TODO
}

void MLProcInputToSignals::setAfterTouch(int note, int value, int time)
{
	#pragma unused(time) // TODO
	// if a voice is playing the given note number,	
	// set aftertouch change for that voice.
	for (int i=0; i<mCurrentVoices; ++i)
	{
		if (note == mVoices[i].mNote)
		{
//			mVoices[i].mdAfter.addChange(value * kControllerScale, time);
			mVoices[i].mdAfter.addChange(value * kControllerScale, 1); // TODO
		}
	}
}

void MLProcInputToSignals::setChannelAfterTouch(int value, int time)
{
	#pragma unused(time) // TODO
	// set aftertouch sum for all voices.
//	mdChannelAfterTouch.addChange(value * kControllerScale, time); // TODO
	mdChannelAfterTouch.addChange(value * kControllerScale, 1);
}

// is any event (key) currently sustained and playing voice v?
bool MLProcInputToSignals::hasHeldKeyEvent(int v)
{
	bool foundEvent = false;
	if (mVoices[v].mNote > 0)
	{				
		// find held key event matching note		
		for (int i=0; i<kMLMaxEvents; ++i)
		{
			if ((mEvents[i].mNote == mVoices[v].mNote) && mEvents[i].isSounding())
			{
				foundEvent = true;
				break;
			}
		}		
	}
	return foundEvent;
}

// TODO ALL MIDI including sustain message should come out of the ring buffer!! order is not guaranteed otherwise.
void MLProcInputToSignals::setSustainPedal(int value, int time)
{
	if(value != mSustain)
	{
		mSustain = value;	
		// for each note:
		if(mSustain)
		{
			// turning on, nothing to do here
		}
		else
		{
			// turn off all sustained voices that do not have key events held down			
			for (int v=0; v<mCurrentVoices; ++v)
			{
				if(!hasHeldKeyEvent(v))
				{
					mVoices[v].mNote = 0;
					mVoices[v].mActive = false;
					mVoices[v].mdAmp.addChange(0.f, time);
				}
			}		
		}
	}
}

