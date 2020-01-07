
#-------------------------------------------------------------------------------
# Includes
#-------------------------------------------------------------------------------

if(VST3_SDK_ROOT)
  MESSAGE(STATUS "VST3_SDK_ROOT=${VST3_SDK_ROOT}")
else()
  MESSAGE(FATAL_ERROR "VST3_SDK_ROOT is not defined. Please use -DVST3_SDK_ROOT=<path to VST3 sdk>.")
endif()

list(APPEND CMAKE_MODULE_PATH "${VST3_SDK_ROOT}/cmake/modules")

include(Global)
include(AddVST3Library)
include(Bundle)
include(ExportedSymbols)
include(PrefixHeader)
include(PlatformIOS)
include(PlatformToolset)
include(CoreAudioSupport)
include(VstGuiSupport)
include(UniversalBinary)
include(AddVST3Options)


#-------------------------------------------------------------------------------
# SDK Project
#-------------------------------------------------------------------------------
setupPlatformToolset()

set(ROOT "${VST3_SDK_ROOT}")

# Here you can define where the VST 3 SDK is located
set(SDK_ROOT "${ROOT}")

# Here you can define where the VSTGUI is located
if(SMTG_ADD_VSTGUI)
  set(VSTGUI_ROOT "${ROOT}")
  setupVstGuiSupport()
endif()

include_directories(${ROOT} ${SDK_ROOT})


#-------------------------------------------------------------------------------
# CORE AUDIO SDK
#-------------------------------------------------------------------------------
setupCoreAudioSupport()


#-------------------------------------------------------------------------------
# Projects
#-------------------------------------------------------------------------------
set(SDK_IDE_LIBS_FOLDER FOLDER "Libraries")

set(VST_SDK TRUE) # used for Base module which provides only a subset of Base for VST-SDK 

set(SDK_IDE_HOSTING_EXAMPLES_FOLDER FOLDER "HostingExamples")
set(SDK_IDE_PLUGIN_EXAMPLES_FOLDER FOLDER "PlugInExamples")

#---Add base libraries---------------------------
add_subdirectory(${VST3_SDK_ROOT}/pluginterfaces vst3-sdk/pluginterfaces)
add_subdirectory(${VST3_SDK_ROOT}/base vst3-sdk/base)
add_subdirectory(${VST3_SDK_ROOT}/public.sdk vst3-sdk/public)
add_subdirectory(${VST3_SDK_ROOT}/public.sdk/source/vst/interappaudio vst3-sdk/interappaudio)
add_subdirectory(${VST3_SDK_ROOT}/public.sdk/samples/vst-hosting/validator vst3-sdk/validator)

#---Add AU wrapper----------------------
if (SMTG_COREAUDIO_SDK_PATH)
  add_subdirectory(${VST3_SDK_ROOT}/public.sdk/source/vst/auwrapper vst3-sdk/auwrapper)
endif()

#-------------------------------------------------------------------------------
# IDE sorting
#-------------------------------------------------------------------------------
set_target_properties(sdk PROPERTIES ${SDK_IDE_LIBS_FOLDER})
set_target_properties(base PROPERTIES ${SDK_IDE_LIBS_FOLDER})

if(SMTG_ADD_VSTGUI)
  set_target_properties(vstgui PROPERTIES ${SDK_IDE_LIBS_FOLDER})
  set_target_properties(vstgui_support PROPERTIES ${SDK_IDE_LIBS_FOLDER})
  set_target_properties(vstgui_uidescription PROPERTIES ${SDK_IDE_LIBS_FOLDER})

  if(TARGET vstgui_standalone)
    set_target_properties(vstgui_standalone PROPERTIES ${SDK_IDE_LIBS_FOLDER})
  endif()
endif()

if(MAC AND XCODE AND SMTG_IOS_DEVELOPMENT_TEAM)
  set_target_properties(base_ios PROPERTIES ${SDK_IDE_LIBS_FOLDER})
endif()
