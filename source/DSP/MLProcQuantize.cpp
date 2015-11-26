
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLScale.h"

// ----------------------------------------------------------------
// class definition

class MLProcQuantize : public MLProc
{
public:
	MLProcQuantize();
	~MLProcQuantize();

	void process(const int frames);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcQuantize> mInfo;
	
	void doParams();
	
	MLScale mScale;
	std::string mScaleName;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcQuantize> classReg("quantize");
	ML_UNUSED MLProcParam<MLProcQuantize> params[2] = { "on", "scale" };	
	ML_UNUSED MLProcInput<MLProcQuantize> inputs[] = { "in" };	
	ML_UNUSED MLProcOutput<MLProcQuantize> outputs[] = { "out" };
}

// ----------------------------------------------------------------
// implementation

MLProcQuantize::MLProcQuantize()
{
	setParam("on", 1);
	mScale.setDefaults();
}

MLProcQuantize::~MLProcQuantize()
{
}

void MLProcQuantize::doParams()
{
	const std::string& scaleName = getStringParam("scale");
	if(scaleName != mScaleName)
	{
		mScale.loadFromRelativePath(scaleName);
		mScaleName = scaleName;
	}
	mParamsChanged = false;
}

void MLProcQuantize::process(const int frames)
{
	if (mParamsChanged) doParams();

	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();

	if(getParam("on"))
	{
		for (int n=0; n < frames; ++n)
		{
			// TODO not every sample OK?
			y[n] = mScale.quantizePitch(x[n]);
		}
	}
	else
	{
		y = x;
	}
}



