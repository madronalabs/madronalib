
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDefaultFileLocations.h"

File getDefaultFileLocation(eFileTypes whichFiles)
{
	File result = File::nonexistent;
	String dest;	
    File startDir;
    
    // get start directory for search according to platform
    if (whichFiles == kAppPresetFiles)
    {
        // app preset files are still in /Library/Application Support/Madrona Labs on Mac
        startDir = File::getSpecialLocation (File::userApplicationDataDirectory);
        startDir = startDir.getChildFile(dest);
    }
    else
    {
    #if JUCE_MAC || JUCE_IOS
        // everything else is now in ~/Music/Madrona Labs on Mac
        String startName = String("~/Music/") + String(MLProjectInfo::makerName);
        startDir = File(startName);
        startDir = startDir.getChildFile(dest);
    #elif JUCE_LINUX || JUCE_ANDROID
        startDir = File("~/" + "." + makerName);
    #elif JUCE_WINDOWS
        startDir = File::getSpecialLocation (File::userApplicationDataDirectory);
        startDir = startDir.getChildFile(dest);
    #endif
    }
    
    // get subdirectory according to file type
	if (startDir.exists())
    {
        switch(whichFiles)
        {
            case kSampleFiles:
                dest = String(MLProjectInfo::projectName) + "/Samples";
                break;
            case kScaleFiles:
                dest = ("Scales");
                break;
            case kPresetFiles:
            case kAppPresetFiles:
                dest = String(MLProjectInfo::projectName);
                break;
        }

        result = startDir.getChildFile(dest);
	
		if (!result.exists())
		{
            result = File::nonexistent;
        }
	}
    
	return result;
}
