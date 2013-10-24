//
//  ProjectInfo.h
//  MLDemoInstrument
//
//  Created by Randy Jones on 8/13/13.
//
//

#ifndef MLDemoInstrument_ProjectInfo_h
#define MLDemoInstrument_ProjectInfo_h

// put #defines from PluginCharacteristics.h into namespace for use in project code.

#include "PluginCharacteristics.h"

namespace MLProjectInfo
{
    const char* const  makerName      = JucePlugin_Manufacturer;
    const char* const  projectName    = JucePlugin_Name;
    const char* const  versionString  = JucePlugin_VersionString;
    const int          versionNumber  = JucePlugin_VersionCode;
}

#endif
