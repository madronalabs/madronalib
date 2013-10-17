
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_DEFAULTFILELOCATIONS_H__
#define __ML_DEFAULTFILELOCATIONS_H__

#include "MLUI.h"
#include "MLDebug.h"
#include "MLProjectInfo.h"

enum eFileTypes
{
	// plugin data
	kFactoryPresetFiles     = 0,  
	kUserPresetFiles,
	kScaleFiles,
	kSampleFiles,
	
	// app persistent state storage
	kAppPresetFiles
};
	

File getDefaultFileLocation(eFileTypes whichFiles);


#endif // __ML_DEFAULTFILELOCATIONS_H__