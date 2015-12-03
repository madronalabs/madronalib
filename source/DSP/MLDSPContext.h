
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
#include "MLSignal.h"
#include "MLClock.h"

class MLProc; // for isProcEnabled. 

class MLDSPContext
{
friend class MLDSPEngine;
public:
	MLDSPContext();
	virtual ~MLDSPContext();

	const MLRatio& getResampleRatio() { return mResampleRatio; }
	const int getResampleUpOrder() { return mResampleUpOrder; }
	const int getResampleDownOrder() { return mResampleDownOrder; }
	inline unsigned getVectorSize() { return mVectorSize; }	
	inline float getSampleRate() { return mSampleRate; }
	inline float getInvSampleRate() { return mInvSampleRate; }
	ml::Time getTime();

	void setResampleRatio(const MLRatio& r) { mResampleRatio = r; }
	void setResampleUpOrder(const int d) { mResampleUpOrder = d; }
	void setResampleDownOrder(const int d) { mResampleDownOrder = d; }
	int setVectorSize(unsigned newSize);
	void setSampleRate(float newRate);	

	MLSignal& getNullInput();
	MLSignal& getNullOutput();

	// ----------------------------------------------------------------
	#pragma mark enable / disable
	//
	virtual void setEnabled(bool t) = 0;
	virtual bool isEnabled() const = 0;
	virtual bool isProcEnabled(const MLProc* p) const = 0;

protected:
	// two null signals: every context has them.
	// the null input is for receiving a signal guaranteed to be 0. 
	// the null output is for dumping unused outputs from procs. 
	MLSignal mNullInput;
	MLSignal mNullOutput;	
	
	bool mEnabled;
	ml::Clock mClock;
	
private:
	MLRatio mResampleRatio;	
	int mResampleUpOrder;	
	int mResampleDownOrder;	
	int mVectorSize;		
	float mSampleRate;	
	float mInvSampleRate;

};


#endif // _ML_DSP_CONTEXT_H