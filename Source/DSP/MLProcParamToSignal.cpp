
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLChangeList.h"

// ----------------------------------------------------------------
// class definition


class MLProcParamToSignal : public MLProc
{
public:
	MLProcParamToSignal();
	~MLProcParamToSignal();

	err resize();

	void clear(){};
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcParamToSignal> mInfo;
	MLChangeList mChangeList;
	MLSample mVal;	
	float mGlide;
	
	// temp
	unsigned mC;
	float mVal1;
};


// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcParamToSignal> classReg("param_to_sig");
	ML_UNUSED MLProcParam<MLProcParamToSignal> params[2] = {"in", "glide"};	
	ML_UNUSED MLProcOutput<MLProcParamToSignal> outputs[] = {"out"};
}

// ----------------------------------------------------------------
// implementation


MLProcParamToSignal::MLProcParamToSignal()
{
	mC = 0;
//	debug() << "MLProcParamToSignal constructor\n";
	setParam("glide", 0.01f);
}


MLProcParamToSignal::~MLProcParamToSignal()
{
//	debug() << "MLProcParamToSignal destructor\n";
}


MLProc::err MLProcParamToSignal::resize()
{
	unsigned vecSize = getContextVectorSize();
	MLSampleRate rate = getContextSampleRate();
	
	mChangeList.setDims(vecSize);
	mChangeList.setSampleRate(rate);
	mChangeList.setGlideTime(mGlide);

	// TODO
	return OK;
}

void MLProcParamToSignal::process(const int frames)
{
	MLSignal& y = getOutput();
	if (mParamsChanged)
	{
		mVal = getParam("in");
		mChangeList.addChange(mVal, 0);
		mGlide = getParam("glide");
		mChangeList.setGlideTime(mGlide);
		mParamsChanged = false;
	}
	
	if (mGlide == 0.f)
	{
		if(mVal1 != mVal)
		{
//			debug() << " #" << std::setprecision(5) << mVal ;
			mVal1 = mVal;
		}
		y.setToConstant(mVal);
	}
	else
	{
		mChangeList.writeToSignal(y, 0, frames);
	}
	
	/*
	std::string name = getName();
	if (!name.compare("pitch_exp_patcher_knob_to_sig"))
	{
		mC++;
		if (mC > 100)
		{
			debug() << "--------------" << getParam("in") << " = " << y[0] <<  "-------------\n" ;
			mC = 0;
		}
	}
	*/
	
}



