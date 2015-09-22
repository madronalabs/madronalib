
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_MULTIPLE_H
#define ML_PROC_MULTIPLE_H

#include "MLProc.h"
#include "MLProcContainer.h"
#include "MLMultProxy.h"

// ----------------------------------------------------------------
// class definition
//
// MLProcMultiple is a kind of container that makes multiple copies 
// of procs that are added to it.  These copies are managed by MLMultProxy objects.
//

class MLProcMultiple : public MLProcContainer
{
public:
	MLProcMultiple();
	~MLProcMultiple();
	
	void doParams();		
	void process(const int samples);

	MLProc::err addProc(const MLSymbol className, const MLSymbol procName);
	MLProcPtr getProc(const MLPath & pathName);
	
	MLProcInfoBase& procInfo() { return mInfo; }
protected:	
 	MLProcInfo<MLProcMultiple> mInfo;	
};


#endif	//ML_PROC_MULTIPLE_H