
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDefaultFileLocations.h"

File getDefaultFileLocation(eFileTypes whichFiles)
{
	File result = File::nonexistent;
	String startStr, destStr;
    File startDir;
    
    // get start directory for search according to platform
    if (whichFiles == kAppPresetFiles)
    {
        // app (not plugin) preset files are still in /Library/Application Support/Madrona Labs on Mac
        startDir = File::getSpecialLocation (File::userApplicationDataDirectory);
    }
    else if (whichFiles == kOldPresetFiles)
    {
        startDir = File::getSpecialLocation (File::userApplicationDataDirectory);
    }
    else
    {
    #if JUCE_MAC || JUCE_IOS
        // everything else is now in ~/Music/Madrona Labs on Mac
        startStr = String("~/Music/") + String(MLProjectInfo::makerName);
        startDir = File(startStr);
    #elif JUCE_LINUX || JUCE_ANDROID
        startDir = File("~/" + "." + makerName);
    #elif JUCE_WINDOWS
        startDir = File::getSpecialLocation (File::userApplicationDataDirectory);
		startDir = startDir.getChildFile(String(MLProjectInfo::makerName));
    #endif
    }
    
    // get subdirectory according to file type
	if (startDir.exists())
    {
        switch(whichFiles)
        {
            case kSampleFiles:
                destStr = String(MLProjectInfo::projectName) + "/Samples";
                break;
            case kScaleFiles:
                destStr = ("Scales");
                break;
            case kOldPresetFiles:
                destStr = String("Audio/Presets/") + String(MLProjectInfo::makerName) + String("/") + String(MLProjectInfo::projectName);
                break;
            case kPresetFiles:
            case kAppPresetFiles:
                destStr = String(MLProjectInfo::projectName);
                break;
        }

        result = startDir.getChildFile(destStr);
	
		if (!result.exists())
		{
            result = File::nonexistent;
        }
	}
    
	/*
    if(result == File::nonexistent)
    {
        debug() << "failed to find location: " << startStr << " / " << destStr << "\n";
    }
    else
    {
        debug() << "found location: " << startStr << " / " << destStr << "\n";
    }
    */
	return result;
}
