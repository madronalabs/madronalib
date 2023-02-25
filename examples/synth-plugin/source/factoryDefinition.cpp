// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#include "public.sdk/source/main/pluginfactory.h"

#include "pluginController.h"
#include "version.h"

//-----------------------------------------------------------------------------
bool InitModule() { return true; }
bool DeinitModule() { return true; }

//-----------------------------------------------------------------------------
// llllPluginNamellll factory definition

#define kVersionString	FULL_VERSION_STR

using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF(stringCompanyName,
				   "llllCompanyURLllll",
				   "lllllCompanyEmailllll")

DEF_CLASS2(INLINE_UID_FROM_FUID(llllpluginnamellll::PluginProcessor::uid),
			PClassInfo::kManyInstances,
			kVstAudioEffectClass,
			"llllPluginNamellll",
			Vst::kDistributable,
			Vst::PlugType::kInstrumentSynth,
			kVersionString,
			kVstVersionString,
			llllpluginnamellll::PluginProcessor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(llllpluginnamellll::PluginController::uid),
			PClassInfo::kManyInstances,
			kVstComponentControllerClass,
			"llllPluginNamellll",
			Vst::kDistributable,
			"",
			kVersionString,
			kVstVersionString,
			llllpluginnamellll::PluginController::createInstance)
//-----------------------------------------------------------------------------
 
END_FACTORY

