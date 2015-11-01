
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
	kPresetFiles     = 0,  
	kScaleFiles,
	kSampleFiles,
	
	// app persistent state storage
	kAppPresetFiles,
    
    // previous location of presets, for converting to Aalto 1.6. Then let's stop the madness.
	kOldPresetFiles,
	kOldPresetFiles2
};

// get the default location for saving files of the given type.
// Creates any needed directories if they do not exist.
// TODO should be MLFile!
File getDefaultFileLocation(eFileTypes whichFiles);

#endif // __ML_DEFAULTFILELOCATIONS_H__