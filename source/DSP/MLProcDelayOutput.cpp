
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLProcContainer.h"
#include "MLProcDelayInput.h"

// ----------------------------------------------------------------
// class definition

class MLProcDelayOutput : public MLProc
{
public:
	 MLProcDelayOutput();
	~MLProcDelayOutput();
	
	void clear();
	err resize();
	void process(const int n);		
	
	MLProcInfoBase& procInfo() { return mInfo; }
private:
	MLProcInfo<MLProcDelayOutput> mInfo;
	
	void doParams();	
	
	MLProcDelayInput* mpDelayInputProc;
	uintptr_t mReadIndex;
	int mVectorDelay;
	uintptr_t mLengthMask;

};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcDelayOutput> classReg("delay_output");
	
	// backwards param can be calculated by compiler TODO
	ML_UNUSED MLProcParam<MLProcDelayOutput> params[] = {"order", "backwards"};	
	ML_UNUSED MLProcInput<MLProcDelayOutput> inputs[] = {"delay_time"}; 
	ML_UNUSED MLProcOutput<MLProcDelayOutput> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation

MLProcDelayOutput::MLProcDelayOutput()
{
	setParam("order", 0);
	setParam("backwards", 0);
	mpDelayInputProc = 0;
	mReadIndex = 0;
	mLengthMask = 0;
}

MLProcDelayOutput::~MLProcDelayOutput()
{
}

void MLProcDelayOutput::clear() 
{	
//	mBuffer.clear();
	mReadIndex = 0;
}

MLProc::err MLProcDelayOutput::resize() 
{
	err e = OK;
	doParams();
	return e;
}


void MLProcDelayOutput::doParams() 
{
	// first get delay input.  That's the MLProcDelayInput object with 
	// the same name as us before the underscore.  This is fairly brittle and
	// hackish.  We don't have something like string parameters, so this
	// is the best I could come up with.
	
	MLProcContainer* myContainer = static_cast<MLProcContainer*>(getContext());
	const std::string myName = getName().getString();
	int dPos = myName.find('_');
	
	MLPath delayName (myName.substr(0, dPos));
	MLProcPtr myInputProc = myContainer->getProc(delayName);
	
	if (myInputProc)
	{
//debug() << "MLProcDelayOutput " << getName() << " doParams found delay proc " << delayName << "!\n";
		mpDelayInputProc = static_cast<MLProcDelayInput*>(&(*myInputProc));
		
		if(mpDelayInputProc)
		{
			// must be power of two size
			MLSignal& buffer = mpDelayInputProc->getBuffer();
			mLengthMask = buffer.getWidth() - 1;
			
//debug() << "MLProcDelayOutput::doParams found buffer, length mask " << std::hex << mLengthMask << std::dec << "\n";
		}
	}
	else
	{
		debug() << "MLProcDelayOutput::doParams: couldn't find delay proc " << delayName << "\n";
	}
	
	if(getParam("backwards"))
	{
		mVectorDelay = getContextVectorSize();
	}
	else
	{
		mVectorDelay = 0;
	}
//debug() << "MLProcDelayOutput: vector delay " << 	mVectorDelay << "\n";
		
	mParamsChanged = false;
}


void MLProcDelayOutput::process(const int frames)
{
	const MLSignal& delayTime = getInput(1);
	MLSignal& y = getOutput();

	int delayInt;
	const float sr = getContextSampleRate();
	MLSample delay;
	
	int delayedIndex;

	if (mParamsChanged) doParams();
	
	if(mpDelayInputProc)
	{
		MLSignal& buffer = mpDelayInputProc->getBuffer();
		if (delayTime.isConstant())
		{
			delay = delayTime[0] * sr - mVectorDelay;
			if (delay < mVectorDelay) delay = mVectorDelay;
			delayInt = (int)(delay);
			
			for (int n=0; n<frames; ++n)
			{
				delayedIndex = mReadIndex - delayInt;
				delayedIndex &= mLengthMask;
				y[n] = buffer[delayedIndex];
				mReadIndex++;
			}
		}
		else
		{
			for (int n=0; n<frames; ++n)
			{		
				// read
				// zero order (integer delay)
				
				// get delay time.  
				// if no signal is attached, 0. should result 
				// and we get a single-vector delay.
				delay = delayTime[n] * sr - mVectorDelay;
				if (delay < mVectorDelay) delay = mVectorDelay;
				
				delayInt = (int)(delay);
				
				delayedIndex = mReadIndex - delayInt;
				delayedIndex &= mLengthMask;
				y[n] = buffer[delayedIndex];
				mReadIndex++;
			}
		}
	}

	// linear interp:
	//y[n] = frac*x[m+1] + (1-frac)*x[m]
	
	// allpass interp:
	// y[n] = x[m+1] + (1-frac)*x[m] - (1-frac)*y[n-1] 
	
}
	   