
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_DEFAULTFILELOCATIONS_H__
#define __ML_DEFAULTFILELOCATIONS_H__

#include "MLUI.h"
#include "MLDebug.h"

enum eFileTypes
{
	// plugin data
	kFactoryPresetFiles     = 0,  
	kUserPresetFiles		= 1,
	kScaleFiles				= 2,
	
	// app storage
	kAppPresetFiles			= 3
};
	

File getDefaultFileLocation(eFileTypes whichFiles);


#endif // __ML_DEFAULTFILELOCATIONS_H__