// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details


#pragma once

#define MAJOR_VERSION_STR "1"
#define MAJOR_VERSION_INT 1

#define SUB_VERSION_STR "0"
#define SUB_VERSION_INT 0

#define RELEASE_NUMBER_STR "0"
#define RELEASE_NUMBER_INT 0

#define BUILD_NUMBER_STR "1"
#define BUILD_NUMBER_INT 1

#define FULL_VERSION_STR "1.0.0"
#define VERSION_STR  FULL_VERSION_STR

#define stringPluginName "llllpluginnamellll"
#define stringOriginalFilename stringPluginName".vst3"

#if SMTG_PLATFORM_64
#define stringFileDescription	"Madronalib example plugin (64Bit)"
#else
#define stringFileDescription	"Madronalib example plugin"
#endif
#define stringCompanyName		"llllCompanyllll"
#define stringLegalCopyright	"llllcopyrightllll"
#define stringLegalTrademarks	"VST is a trademark of Steinberg Media Technologies GmbH"
