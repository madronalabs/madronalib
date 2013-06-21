//
// JuceHeader.h
// 
// --------------------------------------------------------------------------------

#ifndef __JUCE_HEADER_H__
#define __JUCE_HEADER_H__

#include "AppConfig.h"
#include "modules/juce_audio_basics/juce_audio_basics.h"
#include "modules/juce_audio_devices/juce_audio_devices.h"
#include "modules/juce_audio_formats/juce_audio_formats.h"
#include "modules/juce_audio_processors/juce_audio_processors.h"
#include "modules/juce_audio_utils/juce_audio_utils.h"
#include "modules/juce_core/juce_core.h"
#include "modules/juce_data_structures/juce_data_structures.h"
#include "modules/juce_events/juce_events.h"
#include "modules/juce_graphics/juce_graphics.h"
#include "modules/juce_gui_basics/juce_gui_basics.h"
#include "modules/juce_gui_extra/juce_gui_extra.h"
#include "modules/juce_opengl/juce_opengl.h"

namespace ProjectInfo
{
    const char* const  makerName	  = "Madrona Labs";
    const char* const  projectName    = "MadronaLibTests";
    const char* const  versionString  = "1.0";
    const int          versionNumber  = 0x10000;
}

#endif // __JUCE_HEADER_H__
