
// prevent problems including MLPlatform.h
#define TARGET_OS_IPHONE 0
#define TARGET_IPHONE_SIMULATOR 0

#define UseExtendedThingResource 1
#include <AudioUnit/AudioUnit.r>
#include <CoreServices/CoreServices.r>

#include "AppConfig.h"

//==============================================================================
// component resources for Audio Unit
#define RES_ID          1000
#define COMP_TYPE       JucePlugin_AUMainType
#define COMP_SUBTYPE    JucePlugin_AUSubType
#define COMP_MANUF      JucePlugin_AUManufacturerCode
#define VERSION         JucePlugin_VersionCode
#define NAME            JucePlugin_Manufacturer ": " JucePlugin_Name
#define DESCRIPTION     JucePlugin_Desc
#define ENTRY_POINT     JucePlugin_AUExportPrefixQuoted "Entry"


// old AUResources.r is copied here

// this is a define used to indicate that a component has no static data that would mean
// that no more than one instance could be open at a time - never been true for AUs
#ifndef cmpThreadSafeOnMac
#define cmpThreadSafeOnMac	0x10000000
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

resource 'STR ' (RES_ID, purgeable) {
	NAME
};

resource 'STR ' (RES_ID + 1, purgeable) {
	DESCRIPTION
};

resource 'dlle' (RES_ID) {
	ENTRY_POINT
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

resource 'thng' (RES_ID, NAME) {
	COMP_TYPE,
	COMP_SUBTYPE,
	COMP_MANUF,
	0, 0, 0, 0,								//	no 68K
	'STR ',	RES_ID,
	'STR ',	RES_ID + 1,
	0,	0,			/* icon */
	VERSION,
	componentHasMultiplePlatforms | componentDoAutoVersion,
	0,
	{
        #if defined(ppc_YES)
        cmpThreadSafeOnMac,
        'dlle', RES_ID, platformPowerPCNativeEntryPoint
        #define NeedLeadingComma 1
        #endif
        #if defined(ppc64_YES)
		#if defined(NeedLeadingComma)
        ,
		#endif
        cmpThreadSafeOnMac,
        'dlle', RES_ID, platformPowerPC64NativeEntryPoint
        #define NeedLeadingComma 1
        #endif
        #if defined(i386_YES)
		#if defined(NeedLeadingComma)
        ,
		#endif
        cmpThreadSafeOnMac,
        'dlle', RES_ID, platformIA32NativeEntryPoint
        #define NeedLeadingComma 1
        #endif
        #if defined(x86_64_YES)
		#if defined(NeedLeadingComma)
        ,
		#endif
        cmpThreadSafeOnMac,
        'dlle', RES_ID, 8
        #define NeedLeadingComma 1
        #endif
	}
};

#undef RES_ID
#undef COMP_TYPE
#undef COMP_SUBTYPE
#undef COMP_MANUF
#undef VERSION
#undef NAME
#undef DESCRIPTION
#undef ENTRY_POINT
#undef NeedLeadingComma
