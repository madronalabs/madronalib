//
//  MLProcRMS.h
//  Kaivo
//
//  Created by Randy Jones on 6/3/15.
//
//

#ifndef Kaivo_MLProcRMS_h
#define Kaivo_MLProcRMS_h


// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProc.h"
#include "MLDSPUtils.h"

class MLProcRMS : public MLProc
{
public:
	MLProcRMS();
	~MLProcRMS();
	
	void clear(void);		
	void process(const int n);		
	MLProcInfoBase& procInfo() { return mInfo; }
	
	// volume accessor TODO make generic getter via symbol
	float getRMS();	
	
private:
	void calcCoeffs(void);
	MLProcInfo<MLProcRMS> mInfo;
	
	float mRMS;
	MLBiquad mFilter;
	int sampleCounter;
	int snapshotSamples;
};

#endif
