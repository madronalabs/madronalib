// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcInputToSignals.h"

const int kMaxEvents = 64;
const int kNumVoiceSignals = 9;
const char * voiceSignalNames[kNumVoiceSignals] = 
{
	"pitch",
	"gate",
	"amp",
	"vel",
	"voice",
	"after",
	"moda",
	"modb",
	"modc"
};

const float MLProcInputToSignals::kControllerScale = 1.f/127.f;
#if INPUT_DRIFT
    const float kDriftConstants[16] =
    {
        0.465f, 0.005f, 0.013f, 0.019f,
        0.155f, 0.933f, 0.002f, 0.024f,
        0.943f, 0.924f, 0.139f, 0.501f,
        0.196f, 0.591f, 0.961f, 0.442f
    };
    const float MLProcInputToSignals::kDriftConstantsAmount = 0.004f;
    const float MLProcInputToSignals::kDriftRandomAmount = 0.002f;
#endif

// ----------------------------------------------------------------
//
#pragma mark MLVoice
// 

static const int kDriftInterval = 10;

MLVoice::MLVoice()
{
	clearState();
	clearChanges();
}

MLProc::err MLVoice::resize(int bufSize)
{
	MLProc::err ret = MLProc::OK;

	// make delta lists
	// allow for one change each sample, though this is unlikely to get used.
	MLProc::err a = mdPitch.setDims(bufSize);
	MLProc::err j = mdPitchBend.setDims(bufSize);
	MLProc::err i = mdGate.setDims(bufSize);
	MLProc::err b = mdAmp.setDims(bufSize);
	MLProc::err c = mdVel.setDims(bufSize);
	MLProc::err d = mdNotePressure.setDims(bufSize);
	MLProc::err k = mdChannelPressure.setDims(bufSize);
	MLProc::err e = mdMod.setDims(bufSize);
	MLProc::err f = mdMod2.setDims(bufSize);
	MLProc::err g = mdMod3.setDims(bufSize);
	MLProc::err h = mdDrift.setDims(bufSize);
	
	if (a || b || c || d || e || f || g || h || i || j || k)
	{
		ret = MLProc::memErr;
	}
	
	return ret;
}

void MLVoice::clearState()
{
	mState = kOff;
    mInstigatorID = 0;
    mChannel = 0;
	mNote = 0.;
    mAge = 0;
	mStartX = 0.;
	mStartY = 0.;
	mStartVel = 0.;
	mPitch = 0.;
	mX1 = 0.;
	mY1 = 0.;
	mZ1 = 0.;
}

// clear changes but not current state.
void MLVoice::clearChanges()
{
	mdDrift.clearChanges();
	mdPitch.clearChanges();
	mdPitchBend.clearChanges();
	mdGate.clearChanges();
	mdAmp.clearChanges();
	mdVel.clearChanges();
	mdNotePressure.clearChanges();
	mdChannelPressure.clearChanges();
	mdMod.clearChanges();
	mdMod2.clearChanges();
	mdMod2.clearChanges();
}

void MLVoice::zero()
{
	mdDrift.zero();
	mdPitch.zero();
	mdPitchBend.zero();
	mdGate.zero();
	mdAmp.zero();
	mdVel.zero();
	mdNotePressure.zero();
	mdChannelPressure.zero();
	mdMod.zero();
	mdMod2.zero();
	mdMod3.zero();
}

void MLVoice::addNoteEvent(const MLControlEvent& e, const MLScale& scale)
{
	int time = e.mTime;
	int newState;
	float note, gate, vel;
	switch(e.mType)
	{
		case MLControlEvent::kNoteOn:
			newState = kOn;
			note = e.mValue1;
			gate = 1.f;
			vel = e.mValue2;
			break;
		case MLControlEvent::kNoteSustain:
			newState = kSustain;
			note = e.mValue1;
			gate = 1.f;
			vel = e.mValue2;
			break;
		case MLControlEvent::kNoteOff:
		default:
			newState = kOff;
			note = mNote;
			gate = 0.f;
			vel = 0.;
			break;
	}
	
	// set immediate state
    mState = newState;
    mChannel = e.mChannel;
    mInstigatorID = e.mID;
    mNote = note;
    mAge = 0;
	
	// add timed changes to lists
	if(e.mType != MLControlEvent::kNoteSustain)
	{
		mdGate.addChange(gate, time);
		mdVel.addChange(vel, time);
		if(e.mType == MLControlEvent::kNoteOn)
		{
			mdPitch.addChange(scale.noteToLogPitch(note), time);
			mdAmp.addChange(vel, time);
		}
	}
	
	mCurrentNoteEvent = e;
}

void MLVoice::stealNoteEvent(const MLControlEvent& e, const MLScale& scale, bool retrig)
{
 	float note = e.mValue1;
	float vel = e.mValue2;
	int time = e.mTime;
    if (time == 0) time++; // in case where time = 0, make room for retrigger.
    
    mInstigatorID = e.mID;
    mNote = note;
    mAge = 0;
    mdPitch.addChange(scale.noteToLogPitch(note), time);
    
    if (retrig)
    {
        mdGate.addChange(0.f, time - 1);
        mdVel.addChange(0.f, time - 1);
    }
	
    mdGate.addChange(1, time);
    mdAmp.addChange(vel, time);
    mdVel.addChange(vel, time);
	
	mCurrentNoteEvent = e;
}

#pragma mark -
// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcInputToSignals> classReg("midi_to_signals");
	ML_UNUSED MLProcParam<MLProcInputToSignals> params[9] = { "bufsize", "voices", "bend", "mod", "unison", "glide", "protocol", "data_rate" , "scale"};
	// no input signals.
	ML_UNUSED MLProcOutput<MLProcInputToSignals> outputs[] = {"*"};	// variable outputs
}	

MLProcInputToSignals::MLProcInputToSignals() :
    mProtocol(-1),
    mpFrameBuf(0),
    mControllerNumber(-1),
	mCurrentVoices(0),
    mFrameCounter(0),
	mRetrig(false),
	mUnisonInputTouch(-1),
	mUnisonVel(0.),
	mSustainPedal(false)
{
	setParam("voices", 0);	// default
	setParam("protocol", kInputProtocolMIDI);	// default
	setParam("data_rate", 100);	// default
	
	mVoiceRotateOffset = 0;
	mNextEventIdx = 0;
    mEventTimeOffset = 0;
    
	mPitchWheelSemitones = 7.f;
	mUnisonMode = false;
	mRotateMode = true;
	mAddChannelPressure = true;
	mEventCounter = 0;
	mDriftCounter = -1;
	
	temp = 0;
	mOSCDataRate = 100;
    
    mNoteEventsPlaying.resize(kMaxEvents);
    mNoteEventsPending.resize(kMaxEvents);
}

MLProcInputToSignals::~MLProcInputToSignals()
{
}

// set frame buffer for OSC inputs
void MLProcInputToSignals::setInputFrameBuffer(PaUtilRingBuffer* pBuf)
{
	mpFrameBuf = pBuf;
}

// needds to be executed by every process() call to clear changes from change lists.
void MLProcInputToSignals::clearChangeLists()
{
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

	// resize voices
	//
	int bufSize = (int)getParam("bufsize");
	int vecSize = getContextVectorSize();
    
	MLProc::err r;
	for(int i=0; i<kMLEngineMaxVoices; ++i)
	{
		r = mVoices[i].resize(bufSize);
		if (!(r == OK))
        {
            MLError() << "MLProcInputToSignals: resize error!\n";
            break;
        }
	}

	// make signals that apply to all voices
	mTempSignal.setDims(vecSize);
	mChannelAfterTouchSignal.setDims(vecSize);
	
#if defined (__APPLE__)
	if (!mLatestFrame.setDims(MLT3DHub::kFrameWidth, MLT3DHub::kFrameHeight))
	{
		return MLProc::memErr;
	}
#endif

	// make outputs
	//
	for(int i=1; i <= kMLEngineMaxVoices * kNumVoiceSignals; ++i)
	{
		if (!outputIsValid(i))
		{
			setOutput(i, getContext()->getNullOutput());
		}
	}

	// do voice params
	//
	for(int i=0; i<kMLEngineMaxVoices; ++i)
	{
        if((i*kNumVoiceSignals + 1) < getNumOutputs())
        {
            // set initial pitch to 0.
            mVoices[i].mdPitch.addChange(0.f, 0);
            MLSignal& out = getOutput(i*kNumVoiceSignals + 1);
            mVoices[i].mdPitch.writeToSignal(out, vecSize);
            mVoices[i].mdPitchBend.addChange(0.f, 0);
            mVoices[i].mdDrift.setGlideTime(kDriftInterval);
        }
	}

	clearChangeLists();
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

void MLProcInputToSignals::setup()
{
	doParams();
}

void MLProcInputToSignals::doParams()
{
	int newVoices = (int)getParam("voices");
    
    // TODO enable / disable voice containers here
	mOSCDataRate = (int)getParam("data_rate");
	
	const std::string& scaleName = getStringParam("scale");
	mScale.loadFromRelativePath(scaleName);
	
	int newProtocol = (int)getParam("protocol");	
	if (newProtocol != mProtocol)
	{
		mProtocol = newProtocol;
	}
	
	mGlide = getParam("glide");
	for (int v=0; v<kMLEngineMaxVoices; ++v)
	{
		mVoices[v].mdPitch.setGlideTime(mGlide);
		mVoices[v].mdPitchBend.setGlideTime(mGlide);
	}
	
	switch(mProtocol)
	{
		case kInputProtocolOSC:	
			for(int i=0; i<kMLEngineMaxVoices; ++i)
			{
				mVoices[i].mdGate.setGlideTime(0.0f);
				mVoices[i].mdAmp.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdVel.setGlideTime(0.0f);
				mVoices[i].mdNotePressure.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdChannelPressure.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdMod.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdMod2.setGlideTime(1.f / (float)mOSCDataRate);
				mVoices[i].mdMod3.setGlideTime(1.f / (float)mOSCDataRate);
			}
			mAddChannelPressure = false;
			break;
		case kInputProtocolMIDI:	
			for(int i=0; i<kMLEngineMaxVoices; ++i)
			{
				mVoices[i].mdGate.setGlideTime(0.f);
				mVoices[i].mdAmp.setGlideTime(0.001f);
				mVoices[i].mdVel.setGlideTime(0.f);
				mVoices[i].mdNotePressure.setGlideTime(0.001f);
				mVoices[i].mdChannelPressure.setGlideTime(0.001f);
				mVoices[i].mdMod.setGlideTime(0.001f);
				mVoices[i].mdMod2.setGlideTime(0.001f);
				mVoices[i].mdMod3.setGlideTime(0.001f);
			}
			mAddChannelPressure = true;
			break;
	}
	
	if (newVoices != mCurrentVoices)
	{
		mCurrentVoices = newVoices;
		clear();
	}
	
	// pitch wheel mult
	mPitchWheelSemitones = getParam("bend");
	
	// listen to controller number mod
	mControllerNumber = (int)getParam("mod");
	
	int unison = (int)getParam("unison");
	if (mUnisonMode != unison)
	{
		mUnisonMode = unison;
		clear();
	}
	
	mParamsChanged = false;
//dumpParams();	// DEBUG
}

MLProc::err MLProcInputToSignals::prepareToProcess()
{
	clear();
	return OK;
}

void MLProcInputToSignals::clear()
{
	//int bufSize = (int)getParam("bufsize");
	int vecSize = getContextVectorSize();
	
	// debug() << "clearing MLProcInputToSignals: bufsize" << bufSize << ", vecSize " << vecSize << "\n";
	
    clearChangeLists();
	
	for(int i=0; i<kMaxEvents; ++i)
	{
		mNoteEventsPlaying[i].clear();
		mNoteEventsPending[i].clear();
	}
	
	int outs = getNumOutputs();
	if (outs)
	{
		for (int v=0; v<kMLEngineMaxVoices; ++v)
		{
			mVoices[v].clearState();
			mVoices[v].clearChanges();
			mVoices[v].zero();
            
            if((v*kNumVoiceSignals + 9) < outs)
            {                
                mVoices[v].mdPitch.writeToSignal(getOutput(v*kNumVoiceSignals + 1), vecSize);
                mVoices[v].mdGate.writeToSignal(getOutput(v*kNumVoiceSignals + 2), vecSize);
                mVoices[v].mdAmp.writeToSignal(getOutput(v*kNumVoiceSignals + 3), vecSize);
                mVoices[v].mdVel.writeToSignal(getOutput(v*kNumVoiceSignals + 4), vecSize);
                getOutput(v*kNumVoiceSignals + 5).setToConstant(v);
                mVoices[v].mdNotePressure.writeToSignal(getOutput(v*kNumVoiceSignals + 6), vecSize);
                mVoices[v].mdChannelPressure.writeToSignal(getOutput(v*kNumVoiceSignals + 6), vecSize);
                mVoices[v].mdMod.writeToSignal(getOutput(v*kNumVoiceSignals + 7), vecSize);
                mVoices[v].mdMod2.writeToSignal(getOutput(v*kNumVoiceSignals + 8), vecSize);
                mVoices[v].mdMod3.writeToSignal(getOutput(v*kNumVoiceSignals + 9), vecSize);
            }
		}
	}
	mEventCounter = 0;
}

// order of signals:
// pitch
// gate
// amp (gate * velocity)
// vel (velocity, stays same after note off)
// voice 
// aftertouch
// mod, mod2, mod3

// display MIDI: pitch gate vel voice after mod -2 -3 -4
// display OSC: pitch gate vel(constant during hold) voice(touch) after(z) dx dy x y

#pragma mark -

void MLProcInputToSignals::setEventTimeOffset(int t)
{
    mEventTimeOffset = t;
}

void MLProcInputToSignals::setEventRange(MLControlEventVector::const_iterator start, MLControlEventVector::const_iterator end)
{
    mStartEvent = start;
    mEndEvent = end;
}
#pragma mark -

void MLProcInputToSignals::process(const int frames)
{	
	if (mParamsChanged) doParams();
    int sr = getContextSampleRate();
    clearChangeLists();
    
#if INPUT_DRIFT
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
#endif
    
	// update age for each voice
	for (int v=0; v<mCurrentVoices; ++v)
	{
		if(mVoices[v].mAge >= 0)
		{
			mVoices[v].mAge += frames;
		}
	}		

	// generate change lists
	switch(mProtocol)
	{
		case kInputProtocolOSC:	
			processOSC(frames);
			break;
		case kInputProtocolMIDI:	
			processEvents();
			break;
	}
    
	// generate output signals from change lists
    writeOutputSignals(frames);
    
    mFrameCounter += frames;
    if(mFrameCounter > sr)
    {
        mFrameCounter -= sr;
        //dumpEvents();
        //dumpVoices();
        //dumpSignals();
    }
}

// TODO get rid of OSC stuff here.  The wrapper will know about OSC and MIDI, and convert them both into Event lists for the Engine.

void MLProcInputToSignals::processOSC(const int frames)
{	
	float x, y, z, note;
	float dx, dy;
	int avail = 0;
	
	// TODO this code only updates every signal vector (64 samples).  Add sample-accurate
	// reading from OSC.

	// TEMP get most recent frame and apply to whole buffer			
	// read from mpFrameBuf, which is being filled up by OSC listener thread
    
	if(mpFrameBuf)
	{
		avail = PaUtil_GetRingBufferReadAvailable(mpFrameBuf);
		if (avail) do
		{
			PaUtil_ReadRingBuffer(mpFrameBuf, mLatestFrame.getBuffer(), 1);
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

			if (z > 0.f)
			{
				if (mVoices[v].mZ1 <= 0.)
				{
					// turn unison voices on or change unison touch to newest
					mUnisonInputTouch = v;
					ux = mVoices[v].mStartX = x;
					uy = mVoices[v].mStartY = y;
					upitch = mVoices[v].mPitch = mScale.noteToLogPitch(note);
					udx = 0.f;
					udy = 0.f;
					
					// simple velocity
					float dz = z - mVoices[v].mZ1;
                    mVoices[v].mStartVel = clamp(dz*32.f, 0.2f, 1.f);
					
					// store most recent unison start velocity
					mUnisonVel = mVoices[v].mStartVel;
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
				upitch = mScale.noteToLogPitch(note);
				udx = ux - mVoices[mUnisonInputTouch].mStartX;
				udy = uy - mVoices[mUnisonInputTouch].mStartY;
			}
		}

		for (int v=0; v<mCurrentVoices; ++v)
		{			
			const int frameTime = 1;
			mVoices[v].mdPitch.addChange(upitch, frameTime);
			mVoices[v].mdGate.addChange((int)(uz > 0.), frameTime);
			mVoices[v].mdAmp.addChange(uz, frameTime);
			mVoices[v].mdVel.addChange(mUnisonVel, frameTime);
			mVoices[v].mdNotePressure.addChange(udx, frameTime);
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
					mVoices[v].mPitch = mScale.noteToLogPitch(note);
					
					// simple velocity
					float dz = z - mVoices[v].mZ1;
                    mVoices[v].mStartVel = clamp(dz*32.f, 0.2f, 1.f);
					dx = 0.f;
					dy = 0.f;
				}
				else
				{
					// note continues
					mVoices[v].mPitch = mScale.noteToLogPitch(note);
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
					x = mVoices[v].mX1;
					y = mVoices[v].mY1;
				}
			}

			mVoices[v].mZ1 = z;

			const int frameTime = 1;
			mVoices[v].mdPitch.addChange(mVoices[v].mPitch, frameTime);
			mVoices[v].mdGate.addChange((int)(z > 0.), frameTime);
			mVoices[v].mdVel.addChange(mVoices[v].mStartVel, frameTime);
			mVoices[v].mdAmp.addChange(z, frameTime);
			mVoices[v].mdNotePressure.addChange(dx, frameTime);
			mVoices[v].mdMod.addChange(dy, frameTime);
			mVoices[v].mdMod2.addChange(x*2.f - 1.f, frameTime);
			mVoices[v].mdMod3.addChange(y*2.f - 1.f, frameTime);		
		}	
	}	
}

// process control events to make change lists
//
void MLProcInputToSignals::processEvents()
{
    MLControlEvent e;
    MLControlEventVector::const_iterator it;
    for(it = mStartEvent; it != mEndEvent; it++)
    {
        e = *it;
        e.mTime -= mEventTimeOffset;
        processEvent(e);
    }
}

// process one incoming event by making the appropriate changes in state and change lists.
void MLProcInputToSignals::processEvent(const MLControlEvent &event)
{
    switch(event.mType)
    {
        case MLControlEvent::kNoteOn:
            doNoteOn(event);
            break;
        case MLControlEvent::kNoteOff:
            doNoteOff(event);
            break;
        case MLControlEvent::kController:
            doController(event);
            break;
        case MLControlEvent::kPitchWheel:
            doPitchWheel(event);
            break;
        case MLControlEvent::kNotePressure:
            doNotePressure(event);
            break;
        case MLControlEvent::kChannelPressure:
            doChannelPressure(event);
            break;
        case MLControlEvent::kSustainPedal:
            doSustain(event);
            break;
        case MLControlEvent::kNull:
        default:
            break;
    }
}

void MLProcInputToSignals::doNoteOn(const MLControlEvent& event)
{
	// find free event or bail
    int freeEventIdx = mNoteEventsPlaying.findFreeEvent();
    if(freeEventIdx < 0) return;
	mNoteEventsPlaying[freeEventIdx] = event;
	
	// send event to voices
    if(!mUnisonMode)
    {
        int v = findFreeVoice();
        if(v >= 0)
        {
            mVoices[v].addNoteEvent(event, mScale);
        }
        else
        {
            // find a sustained voice to steal
			v = findOldestSustainedVoice();
            if(v < 0)
            {
				// or failing that, the oldest
                v = findOldestVoice();
            }
			mVoices[v].stealNoteEvent(event, mScale, true);
        }
    }
    else
    {
		// push any event previously occupying voices to pending stack
		// assuming all voices are playing the same event.
		if (mVoices[0].mState == MLVoice::kOn)
		{
			const MLControlEvent& prevEvent = mVoices[0].mCurrentNoteEvent;
			mNoteEventsPending.push(prevEvent);
			mNoteEventsPlaying.clearEventsMatchingID(prevEvent.mID);
		}
		for (int v = 0; v < mCurrentVoices; ++v)
		{
            mVoices[v].addNoteEvent(event, mScale);
		}
    }
}

void MLProcInputToSignals::doNoteOff(const MLControlEvent& event)
{
	// clear all events matching instigator
    int instigator = event.mID;
	for (int i=0; i<kMaxEvents; ++i)
	{
		if(mNoteEventsPlaying[i].mID == instigator)
		{
			mNoteEventsPlaying[i].clear();
		}
	}
	
	if (!mUnisonMode)
	{
		// send either off or sustain event to voices matching instigator
		MLControlEvent::EventType newEventType = mSustainPedal ? MLControlEvent::kNoteSustain : MLControlEvent::kNoteOff;
		for(int v=0; v<mCurrentVoices; ++v)
		{
			MLVoice& voice = mVoices[v];
			if(voice.mInstigatorID == instigator)
			{
				MLControlEvent eventToSend = event;
				eventToSend.mType = newEventType;
				voice.addNoteEvent(eventToSend, mScale);
			}
		}
	}
	else // unison
	{
		// if note off is the sounding event,
		// play the most recent note from pending stack, or release or sustain last note.
		// else delete the note from events and pending stack.
		if(mVoices[0].mInstigatorID == instigator)
		{
			if(!mNoteEventsPending.isEmpty())
			{
				MLControlEvent pendingEvent = mNoteEventsPending.pop();
				for (int v = 0; v < mCurrentVoices; ++v)
				{
					mVoices[v].stealNoteEvent(pendingEvent, mScale, mRetrig);
				}
			}
			else
			{
				// release or sustain
				MLControlEvent::EventType newEventType = mSustainPedal ? MLControlEvent::kNoteSustain : MLControlEvent::kNoteOff;
				for(int v=0; v<mCurrentVoices; ++v)
				{
					MLVoice& voice = mVoices[v];
					MLControlEvent eventToSend = event;
					eventToSend.mType = newEventType;
					voice.addNoteEvent(eventToSend, mScale);
				}
			}
		}
		else
		{
			mNoteEventsPending.clearEventsMatchingID(instigator);
		}
	}
}

void MLProcInputToSignals::doSustain(const MLControlEvent& event)
{
    mSustainPedal = (int)event.mValue1;
    if(!mSustainPedal)
    {
        // clear any sustaining voices
		for(int i=0; i<mCurrentVoices; ++i)
		{
			MLVoice& v = mVoices[i];
			if(v.mState == MLVoice::kSustain)
			{
				MLControlEvent newEvent;
				newEvent.mType = MLControlEvent::kNoteOff;
				v.addNoteEvent(newEvent, mScale);
			}
		}
    }
}

void MLProcInputToSignals::doController(const MLControlEvent& event)
{
    int time = event.mTime;
	float ctrl = event.mValue1;
	float val = event.mValue2;
	for (int i=0; i<mCurrentVoices; ++i)
	{
		if (event.mChannel == mVoices[i].mChannel)
		{
            if (ctrl == mControllerNumber)
            {
                mVoices[i].mdMod.addChange(val, time);
            }
            else if (ctrl == mControllerNumber + 1)
            {
                mVoices[i].mdMod2.addChange(val, time);
            }
            else if (ctrl == mControllerNumber + 2)
            {
                mVoices[i].mdMod3.addChange(val, time);
            }
        }
    }
}

void MLProcInputToSignals::doPitchWheel(const MLControlEvent& event)
{
	float val = event.mValue1;
    float ctr = val - 8192.f;
    float u = ctr / 8191.f;
	float bendAdd = u * mPitchWheelSemitones / 12.f;
    
	for (int i=0; i<mCurrentVoices; ++i)
	{
		if (event.mChannel == mVoices[i].mChannel)
		{
            mVoices[i].mdPitchBend.addChange(bendAdd, event.mTime);
		}
	}
}

void MLProcInputToSignals::doNotePressure(const MLControlEvent& event)
{
	debug() << "NOTE PRESSURE: ID:" << event.mID << " channel:" << event.mChannel << " val:" << event.mValue2 << "\n";
	for (int i=0; i<mCurrentVoices; ++i)
	{
		if ((event.mChannel == mVoices[i].mChannel) && (event.mID == mVoices[i].mInstigatorID))
		{
			mVoices[i].mdNotePressure.addChange(event.mValue2, event.mTime);
		}
	}
}

void MLProcInputToSignals::doChannelPressure(const MLControlEvent& event)
{
	for (int i=0; i<mCurrentVoices; ++i)
	{
		if (event.mChannel == mVoices[i].mChannel)
		{
			mVoices[i].mdChannelPressure.addChange(event.mValue1, event.mTime);
		}
	}
}

// process change lists to make output signals
//
void MLProcInputToSignals::writeOutputSignals(const int frames)
{
	for (int v=0; v<kMLEngineMaxVoices; ++v)
	{
		// changes per voice
		MLSignal& pitch = getOutput(v*kNumVoiceSignals + 1);
		MLSignal& gate = getOutput(v*kNumVoiceSignals + 2);
		MLSignal& amp = getOutput(v*kNumVoiceSignals + 3);
		MLSignal& velSig = getOutput(v*kNumVoiceSignals + 4);
		MLSignal& voiceSig = getOutput(v*kNumVoiceSignals + 5);
		MLSignal& after = getOutput(v*kNumVoiceSignals + 6);
		MLSignal& mod = getOutput(v*kNumVoiceSignals + 7);
		MLSignal& mod2 = getOutput(v*kNumVoiceSignals + 8);
		MLSignal& mod3 = getOutput(v*kNumVoiceSignals + 9);

		if (v < mCurrentVoices)
		{
			// write signals
			mVoices[v].mdPitch.writeToSignal(pitch, frames);
			mVoices[v].mdPitchBend.writeToSignal(mTempSignal, frames);
			pitch.add(mTempSignal);
#if INPUT_DRIFT
			// write to common temp drift signal, we add one change manually so read offset is 0
			mVoices[v].mdDrift.writeToSignal(mTempSignal, frames);
			pitch.add(mTempSignal);
#endif            
			mVoices[v].mdGate.writeToSignal(gate, frames);
            mVoices[v].mdAmp.writeToSignal(amp, frames);
 			mVoices[v].mdVel.writeToSignal(velSig, frames);
			voiceSig.setToConstant(v);
			
			// aftertouch for each voice is channel aftertouch + poly aftertouch.
			mVoices[v].mdNotePressure.writeToSignal(after, frames);
			if(mAddChannelPressure)
			{
				mVoices[v].mdChannelPressure.writeToSignal(mTempSignal, frames);
				after.add(mTempSignal);
			}
			mVoices[v].mdMod.writeToSignal(mod, frames);
 			mVoices[v].mdMod2.writeToSignal(mod2, frames);
 			mVoices[v].mdMod3.writeToSignal(mod3, frames);
            
			// clear change lists
			mVoices[v].mdPitch.clearChanges();
			mVoices[v].mdPitchBend.clearChanges();
			mVoices[v].mdGate.clearChanges();
			mVoices[v].mdAmp.clearChanges();
			mVoices[v].mdVel.clearChanges();
			mVoices[v].mdNotePressure.clearChanges();
			mVoices[v].mdChannelPressure.clearChanges();
			mVoices[v].mdMod.clearChanges();
			mVoices[v].mdMod2.clearChanges();
			mVoices[v].mdMod3.clearChanges();
#if INPUT_DRIFT
			mVoices[v].mdDrift.clearChanges();
#endif
		}
		else
		{
			pitch.setToConstant(0.f);
			gate.setToConstant(0.f);
			amp.setToConstant(0.f);
			velSig.setToConstant(0.f);
			voiceSig.setToConstant(0.f);
			after.setToConstant(0.f);
			mod.setToConstant(0.f); 
			mod2.setToConstant(0.f); 
			mod3.setToConstant(0.f); 
		}
	}
}

#pragma mark -

// return index of free voice or -1 for none.
// increments mVoiceRotateOffset.
//
int MLProcInputToSignals::findFreeVoice()
{
	int r = -1;
	for (int v = 0; v < mCurrentVoices; ++v)
	{
		int vr = v;
        if(mRotateMode)
        {
            vr = (vr + mVoiceRotateOffset) % mCurrentVoices;
        }
        if (mVoices[vr].mState == MLVoice::kOff)
		{
			r = vr;
            mVoiceRotateOffset++;
			break;
		}
	}
	return r;
}

int MLProcInputToSignals::findOldestSustainedVoice()
{
	int r = -1;
	std::list<int> sustainedVoices;
	for (int i=0; i<mCurrentVoices; ++i)
	{
		MLVoice& v = mVoices[i];
		if(v.mState == MLVoice::kSustain)
		{
			sustainedVoices.push_back(i);
		}
	}
	
    int maxAge = -1;
	for(std::list<int>::const_iterator it = sustainedVoices.begin();
		it != sustainedVoices.end(); it++)
	{
        int voiceIdx = *it;
		int age = mVoices[voiceIdx].mAge;
        if (age > maxAge)
        {
            maxAge = age;
            r = voiceIdx;
        }
	}
	return r;
}

int MLProcInputToSignals::findOldestVoice()
{
	int r = 0;
    int maxAge = -1;
    for (int v=0; v<mCurrentVoices; ++v)
    {
        int age = mVoices[v].mAge;
        if (age > maxAge)
        {
            maxAge = age;
            r = v;
        }
    }
	return r;
}

void MLProcInputToSignals::dumpEvents()
{
	for (int i=0; i<kMaxEvents; ++i)
	{
		MLControlEvent& event = mNoteEventsPlaying[i];
		int type = event.mType;
		switch(type)
		{
		case MLControlEvent::kNull:
			debug() << "-";
			break;		
		case MLControlEvent::kNoteOn:
			debug() << "N";
			break;		
		default:
			debug() << "?";
			break;
		}
	}
	debug() << "\n";
	int pendingSize = mNoteEventsPending.getSize();
	for(int i = 0; i < pendingSize; ++i)
	{
		debug() << mNoteEventsPending[i].mID << " ";
	}
	debug() << "\n";
}

void MLProcInputToSignals::dumpVoices()
{
	for (int i=0; i<mCurrentVoices; ++i)
	{
		MLVoice& voice = mVoices[i];
        switch(voice.mState)
        {
			case MLVoice::kOff:
				debug() << ".";
				break;
			case MLVoice::kOn:
				debug() << "*";
				break;
			case MLVoice::kSustain:
				debug() << "s";
				break;
			default:
				debug() << " ";
				break;
        }
	}
	debug() << "\n";
}

void MLProcInputToSignals::dumpSignals()
{
	for (int i=0; i<mCurrentVoices; ++i)
	{
        debug() << "voice " << i << ": ";
        
        // changes per voice
        MLSignal& pitch = getOutput(i*kNumVoiceSignals + 1);
        MLSignal& gate = getOutput(i*kNumVoiceSignals + 2);
        MLSignal& amp = getOutput(i*kNumVoiceSignals + 3);
        MLSignal& vel = getOutput(i*kNumVoiceSignals + 4);

        debug() << "[pitch: " << pitch[0] << "] ";
        debug() << "[gate : " << gate[0] << "] ";
        debug() << "[amp  : " << amp[0] << "] ";
        debug() << "[vel  : " << vel[0] << "] ";
        debug() << "\n";
    }
	debug() << "\n";
}
