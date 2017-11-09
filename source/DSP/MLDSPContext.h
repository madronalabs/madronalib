
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// an MLProc can only be run inside an MLDSPContext.  While MLProcContainer
// holds procs and static connections between them, an MLDSPContext provides the 
// info needed to make the graph dynamic. 
//
// practically, this class exists so that MLProc can include it to get info
// about buffer size, sample rate, etc.  MLProc doesn't want to include 
// MLProcContainer. 

// TODO we would like any proc to be able to look up resources, and for those resources to be
// shared between procs. To this end make an MLDSPDevice object that will hold all resources.
// One such object (singleton) services one application in which DSP is happening, essentially. 
// or it could be called DSPResourceLibrary or something.

#ifndef _ML_DSP_CONTEXT_H
#define _ML_DSP_CONTEXT_H

#include "MLRatio.h"
#include "core/MLSignal.h"
#include "MLClock.h"
#include "MLPropertySet.h"

class MLProc; // for isProcEnabled. 
class MLDSPEngine;

class MLDSPContext : 
public MLPropertySet // DSPResourceLibrary placeholder
{
friend class MLDSPEngine;
public:
	MLDSPContext();
	virtual ~MLDSPContext();

	inline unsigned getVectorSize() { return mVectorSize; }	
	inline float getSampleRate() { return mSampleRate; }
	inline float getInvSampleRate() { return mInvSampleRate; }
	ml::Time getTime();

	int setVectorSize(unsigned newSize);
	void setSampleRate(float newRate);	

	MLSignal& getNullInput();
	MLSignal& getNullOutput();

	// ----------------------------------------------------------------
	#pragma mark enable / disable

	virtual void setEnabled(bool t) = 0;
	virtual bool isEnabled() const = 0;
	virtual bool isProcEnabled(const MLProc* p) const = 0;

	// ----------------------------------------------------------------
	#pragma mark input / output limits
	
	static const int kMaxSigs = 128;
	virtual int getMaxInputSignals() { return kMaxSigs; }
	virtual int getMaxOutputSignals() { return kMaxSigs; }

	// ----------------------------------------------------------------
	#pragma root context pointer
	
	void setRootContext(MLDSPContext* pC) { mpRootContext = pC; }
	MLDSPContext* getRootContext() { return mpRootContext; }
	
	
	// only used for root engine. TODO move to Engine.
	
	void setMaxVoices(int v) { mMaxVoices = v; }
	int getMaxVoices() { return mMaxVoices; }

	
	// ----------------------------------------------------------------
	#pragma resources

protected:
	// two null signals: every context has them.
	// the null input is for receiving a signal guaranteed to be 0. 
	// the null output is for dumping unused outputs from procs. 
	MLSignal mNullInput;
	MLSignal mNullOutput;	
	
	int mMaxVoices;

	bool mEnabled;
	ml::Clock mClock;
	
	// every context has a pointer up to its root engine.
	MLDSPContext* mpRootContext;
	
private:
	int mVectorSize;		
	float mSampleRate;	
	float mInvSampleRate;

};


#endif // _ML_DSP_CONTEXT_H
