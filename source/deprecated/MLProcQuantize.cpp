
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLScale.h"
#include "MLDSPDeprecated.h"

typedef enum
{
	kTruncate = 0,
	kNearest = 1
} eQuantizeMode;

// ----------------------------------------------------------------
// class definition

class MLProcQuantize : public MLProc
{
public:
	MLProcQuantize();
	~MLProcQuantize();

	void process() override;		
	MLProcInfoBase& procInfo() override { return mInfo; }

private:
	MLProcInfo<MLProcQuantize> mInfo;
	
	void doParams();
	
	MLBiquad mPitchFilter; // TODO onepole
	
	MLScale mScale;
	int mMode;
	int mCounter;
	float mNewPitch;
	ml::Text mScaleName;
};

// ----------------------------------------------------------------
// registry section

namespace
{
	MLProcRegistryEntry<MLProcQuantize> classReg("quantize");
	ML_UNUSED MLProcParam<MLProcQuantize> params[] = { "on", "scale", "mode" };	
	ML_UNUSED MLProcInput<MLProcQuantize> inputs[] = { "in" };	
	ML_UNUSED MLProcOutput<MLProcQuantize> outputs[] = { "out" };
}

// ----------------------------------------------------------------
// implementation

MLProcQuantize::MLProcQuantize()
{
	setParam("on", 1);
	setParam("mode", kTruncate);
	mScale.setDefaults();
	mCounter = 0;
	mNewPitch = 0.f;
}

MLProcQuantize::~MLProcQuantize()
{
}

void MLProcQuantize::doParams()
{
	static const ml::Symbol scaleSym("scale");
	static const ml::Symbol modeSym("mode");

	ml::Text scaleName = getTextParam(scaleSym);
	if(scaleName != mScaleName)
	{
		// what about mapping?
		mScale.loadFromRelativePath(scaleName);
		
		// TODO
	//	std::string scaleText = getContext()->loadStringFromPath(scaleName);
	//	mScale.loadScaleFromString(scaleText);
		
		
		mScaleName = scaleName;
	}
	mMode = static_cast<int>(getParam(modeSym));
	mParamsChanged = false;
}

void MLProcQuantize::process()
{
	static const ml::Symbol onSym("on");
	
	if (mParamsChanged) doParams();

	const MLSignal& x = getInput(1);
	MLSignal& y = getOutput();
	
	if(!getParam(onSym))
	{
		y = x; 
		return;
	}
	
	// TODO move
	mPitchFilter.setSampleRate(getContextSampleRate());
	mPitchFilter.setOnePole(100.f);

	for (int n=0; n < kFloatsPerDSPVector; ++n)
	{
		mCounter++;
		mCounter &= 0xFF;
		
		if(!mCounter)
		{
			if(mMode == kTruncate)
			{
				mNewPitch = mScale.quantizePitch(x[n]);
			}
			else
			{
				mNewPitch = mScale.quantizePitchNearest(x[n]);
			}
		}
		
		float r = mPitchFilter.processSample(mNewPitch);
		y[n] = r;
	}
}



