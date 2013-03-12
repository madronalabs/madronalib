
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDefaultFileLocations.h"

File getDefaultFileLocation(eFileTypes whichFiles)
{
	File result = File::nonexistent;
	String dest;	

	// get start directory for search
	File::SpecialLocationType startDirType;
	switch(whichFiles)
	{
		case kFactoryPresetFiles: case kScaleFiles:
			startDirType = File::commonApplicationDataDirectory;
		break;
		case kUserPresetFiles:
			startDirType = File::userApplicationDataDirectory;	
		break;
		default:
		break;
	}
	File startDir = File::getSpecialLocation (startDirType);	
	

	if (startDir.exists())
	{

#if JUCE_WINDOWS

		// debug() << "Start dir: " << startDir.getFullPathName() << " \n";

		if (whichFiles == kScaleFiles)
		{
			dest = String(kMLJuceAppMaker) + "/Scales";
		}
		else if (whichFiles == kFactoryPresetFiles)
		{
			dest = String(kMLJuceAppMaker) + "/" + ProjectInfo::projectName + "/Presets";
		}
		else if (whichFiles == kUserPresetFiles)
		{
			dest = String(kMLJuceAppMaker) + "/" + ProjectInfo::projectName + "/Presets";
		}

#elif JUCE_LINUX


#elif JUCE_IPHONE


#elif JUCE_MAC
		if (whichFiles == kScaleFiles)
		{
			dest = "Audio/Presets/Madrona Labs/Scales";
		}
		else // either presets type
		{
			dest = String("Audio/Presets/Madrona Labs/") + ProjectInfo::projectName;
		}
#endif
		result = startDir.getChildFile(dest);	
		/*
		if (result.exists())
		{
			debug() << "found path: " << dest << "\n";
			debug() << "full path:" << result.getFullPathName() << "\n";
		}*/
	}
	return result;
}
