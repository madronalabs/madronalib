
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_HOST_PHASOR_H
#define ML_PROC_HOST_PHASOR_H

#include "MLProc.h"

// ----------------------------------------------------------------
// class definition

class MLProcHostPhasor : public MLProc
{
public:
	 MLProcHostPhasor();
	~MLProcHostPhasor();

	void setTimeAndRate(const double secs, const double position, const double bpm, bool isPlaying);
	
	void clear();
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	MLProcInfo<MLProcHostPhasor> mInfo;
	void calcCoeffs(void);
	MLSample mOmega;
	MLSample mdOmega;
	double mTime;
	double mRate;
	bool mPlaying;
	bool mActive;
};

#endif // ML_PROC_HOST_PHASOR_H