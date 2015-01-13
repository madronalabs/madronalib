
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcEnvelope : public MLProc
{
public:
	 MLProcEnvelope();
	~MLProcEnvelope();
	
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcEnvelope> mInfo;
	void calcCoeffs(void);
		
	MLSample mEnvThresh;	// coeffs
	MLSample mDelayCounter, mDelayCounterStep, mDelayStep;
	MLSample mRepeatCounter, mRepeatStep;
	MLSample mSustain;
	MLSample mCAttack, mCDecay, mCSustain, mCRelease, mCNull; // coeffs
	MLSample mX;	// input to filter
	MLSample mGate1, mEnv, mY1; // history
	MLSample mMult;	// output multiply by gate amp if xvel is on, or 1 if xvel is off
	MLSample* mpEnvCoeff; // points to current step being used
	
	enum {stateOff, stateDelay, stateAttack, stateDecay, stateSustain, stateRelease}; // envelope states
	int mState;
	int mT;
};
	
static const float kMinSegTime = 0.0002f;

// ----------------------------------------------------------------
// registry section

namespace{

MLProcRegistryEntry<MLProcEnvelope> classReg("envelope");
ML_UNUSED MLProcParam<MLProcEnvelope> params[] = { "xvel", "trig_select" };
ML_UNUSED MLProcInput<MLProcEnvelope> inputs[] = {"in",  "delay", "attack", "decay", "sustain", "release", "repeat", "vel" };
ML_UNUSED MLProcOutput<MLProcEnvelope> outputs[] = {"out"};

}	// namespace

// ----------------------------------------------------------------
// implementation

MLProcEnvelope::MLProcEnvelope()
{
}

MLProcEnvelope::~MLProcEnvelope()
{
}

void MLProcEnvelope::calcCoeffs(void) 
{
	mParamsChanged = false;
}

void MLProcEnvelope::clear()
{
	// debug() << "MLProcEnvelope::clear()\n";
	mEnvThresh = 1.f;
	mpEnvCoeff = &mCNull; 
	mX = mGate1 = mEnv = mY1 = 0.f;
	mDelayCounter = mDelayCounterStep = mDelayStep = 0.f;
	mRepeatCounter = mRepeatStep = 0.f;
	mCAttack = mCDecay = mCSustain = mCRelease = mCNull = 0.f;
	mState = stateOff;
	mMult = 1.f;
	mT = 0;
}

// generate envelope output based on gate and control signal inputs.
void MLProcEnvelope::process(const int samples)
{	
	float invSr = getContextInvSampleRate();
	const MLSignal& gate = getInput(1);
	const MLSignal& delay = getInput(2);
	const MLSignal& attack = getInput(3);
	const MLSignal& decay = getInput(4);
	const MLSignal& sustain = getInput(5);
	const MLSignal& release = getInput(6);
	const MLSignal& repeat = getInput(7);
	const MLSignal& vel = getInput(8);
	MLSignal& y = getOutput();
	
	static MLSymbol trigSelectSym("trig_select");
	const bool trigSelect = getParam(trigSelectSym) > 1.f; // this param is 1 or 2
	
	static MLSymbol xvelSym("xvel");
	const bool doMult = (getParam(xvelSym) > 0.f) && !trigSelect;
    
	// input change thresholds for state changes
	const float inputThresh = 0.001f;

	for (int n=0; n<samples; ++n)
	{
        float bias = 0.05f;
        float dxdt, gIn, velIn;
        bool upTrig, downTrig, crossedThresh, delayCounterDone, doRepeat;
        
        // TEMP
        float attackIn = attack[n] - 0.0001f;
        attackIn = clamp(attackIn, 0.f, 20.f);
		
		// TODO make constant coefficient vectors for constant input parameter signals.
		// TODO mark/write output signal as constant when we know it is.		
		// get step sizes(divide approx. will be just fine)
		// SSE here works on a single instance
		{
			mSustain = sustain[n];
			mDelayStep = invSr / max(delay[n], kMinSegTime); 
			mRepeatStep = (repeat[n] == 0.f) ? 0.f : invSr / max(repeat[n], kMinSegTime);
			mCAttack =  kMLTwoPi * invSr / max(attackIn, kMinSegTime);
			mCDecay = kMLTwoPi * invSr / max(decay[n], kMinSegTime);
//			mCSustain = kMLTwoPi * invSr / ();
			mCRelease = kMLTwoPi * invSr / max(release[n], kMinSegTime);
		}
		
		// process gate input
		gIn = gate[n];
		velIn = vel[n];
		const bool wasOver = mGate1 > inputThresh;
		const bool isOver = gIn > inputThresh;		
		upTrig = !wasOver && isOver;
		downTrig = wasOver && !isOver;
		
		// IIR filter.  
		dxdt = mX - mEnv;
		mEnv += dxdt * (*mpEnvCoeff);
		
		// linear counters. TODO use integers
		// TODO change repeat interval during repeat for knob or signal change
		mDelayCounter += mDelayCounterStep;
		mRepeatCounter += (mState == stateDelay) ? 0.f : mRepeatStep;
		
		// did env cross threshold in either direction?
		crossedThresh = (sign(mEnv - mEnvThresh) != sign(mY1 - mEnvThresh));		
		delayCounterDone = mDelayCounter > 1.0;
		// no repeat when sustain is > thresh. (why?)
		doRepeat = ((mSustain < 0.05f) && (mRepeatCounter > 1.0) && (mRepeatStep > 0.));
		
		// did something happen?  usually it doesn't, so we wrap all the branches in just one outer branch.
		if (upTrig || downTrig || delayCounterDone || doRepeat || crossedThresh)
		{
			if (upTrig)  // start delay
			{
				mDelayCounterStep = mDelayStep;
				mDelayCounter = 0.f;		
				mEnvThresh = 0.f;
				mpEnvCoeff = &mCNull;
				mEnv = 0.f;
				mX = 0.f;
				mState = stateDelay;
				mMult = doMult ? velIn : 1.f;	// set mMult here only.
			}
			else if (delayCounterDone || doRepeat) // start attack
			{	
				mRepeatCounter = 0.f;				
				mDelayCounterStep = 0.f;				
				mDelayCounter = 0.f;
				mEnvThresh = 1.f;
				mX = 1.f + bias;
				mpEnvCoeff = &mCAttack; 
				mState = stateAttack;
			}
			else if (downTrig) // go to release
			{	
				// cancel repeat
		//		mRepeatCounterStep = 0.;
		//		mRepeatCounter = 0.;
				
				// cancel delay
				mDelayCounter = 0.f;
				mDelayCounterStep = 0.f;
				
				// release is defined as time to fall to 0 from a value of 1.0.
				mpEnvCoeff = &mCRelease;
				mEnvThresh = 0.f;
				mX = 0.f - bias;
				mState = stateRelease;
			}		
			else if (crossedThresh)
			{
				switch(mState)
				{
					case stateDelay: // go to attack
					break;
					case stateAttack: // go to decay
						mpEnvCoeff = &mCDecay; 
						mEnvThresh = mSustain;
						mX = mSustain - bias;
						mState = stateDecay;
					break;
					case stateDecay: // go to sustain
						mpEnvCoeff = &mCNull; 
						mState = stateSustain;
					break;
					case stateSustain:
						// TODO go to new sustain value if level param changes
						mpEnvCoeff = &mCNull; 
					break;
					case stateRelease: // stop at 0
						mpEnvCoeff = &mCNull; 
						mEnvThresh = 0.f;
						mState = stateOff;
					default:
					break;
				}
			}
		}
		
		mGate1 = gIn;
		mY1 = mEnv; // history is of linear ramp, before clip and scale
		mEnv = clamp(mEnv, 0.f, 1.f); // could be avoided by careful attention to overshoots > 1 and < 0		
		y[n] = mEnv * mMult * 2.f;
	}
	
	/*
	int sr = getContextSampleRate();
	mT += samples;
	if (mT > sr)
	{
	//	debug() << getName() << " state: " << mState << " env: " << mEnv << "\n";
		debug() << "trig: " << trigSelect << "\n";
		mT -= sr;
	}
	*/
}

// TODO envelope sometimes sticks on for very fast gate transients.  Rewrite this thing!



   