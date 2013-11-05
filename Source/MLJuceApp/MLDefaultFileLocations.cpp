
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDefaultFileLocations.h"

File getDefaultFileLocation(eFileTypes whichFiles)
{
	File result = File::nonexistent;
	String dest;	

	// get start directory for search according to platform and file type
	File::SpecialLocationType startDirType;
    File startDir;
	switch(whichFiles)
	{
		case kPresetFiles:
        case kScaleFiles:         
		case kSampleFiles:
            startDir = File::getSpecialLocation (File::userApplicationDataDirectory);
            break;
        case kAppPresetFiles:
		default:
#if JUCE_MAC || JUCE_IOS
            startDir = File("~/Library/Application Support"); // user Library, not system
#elif JUCE_LINUX || JUCE_ANDROID
            startDir = File("~/" + "." + makerName + "/" + applicationName);
#elif JUCE_WINDOWS
            startDir = File::getSpecialLocation (File::userApplicationDataDirectory);
#endif
			break;
	}
	
    // get subdirectory 
	if (startDir.exists())
	{
#if JUCE_WINDOWS

		// debug() << "Start dir: " << startDir.getFullPathName() << " \n";

		if (whichFiles == kScaleFiles)
		{
			dest = String(MLProjectInfo::makerName) + "/Scales";
		}
		else if (whichFiles == kSampleFiles)
		{
			dest = String(MLProjectInfo::makerName) + "/" + MLProjectInfo::projectName + "/Samples";
		}
		else if (whichFiles == kPresetFiles)
		{
			dest = String(MLProjectInfo::makerName) + "/" + MLProjectInfo::projectName + "/Presets";
		}
		else if (whichFiles == kAppPresetFiles)
		{
			dest = String(MLProjectInfo::makerName) + "/" + MLProjectInfo::projectName ;
		}

#elif JUCE_LINUX


#elif JUCE_IPHONE


#elif JUCE_MAC
		if (whichFiles == kScaleFiles)
		{
			dest = String("Audio/Presets/") + MLProjectInfo::makerName + "/Scales";
		}
		else if (whichFiles == kSampleFiles)
		{
			dest = String("Audio/Presets/") + MLProjectInfo::makerName + "/" + MLProjectInfo::projectName + "/Samples";
		}
		else if (whichFiles == kPresetFiles)
        {
			dest = String("Audio/Presets/") + MLProjectInfo::makerName + "/" + MLProjectInfo::projectName;
		}
		else if (whichFiles == kAppPresetFiles)
		{
			dest = String(MLProjectInfo::makerName) + "/" + MLProjectInfo::projectName ;
		}
#endif

        result = startDir.getChildFile(dest);
	
		if (!result.exists())
		{
            result = File::nonexistent;
        }
	}
	return result;
}
