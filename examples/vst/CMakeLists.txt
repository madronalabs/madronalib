# root CMakeLists.txt for madronalib vst3 sample built out of source.
# to run, set VST3_SDK_ROOT to inform CMake where the VST3 SDK is, for example:
# mkdir build
# cd build
# cmake -DVST3_SDK_ROOT=/Users/randy/dev/vst3sdk -DCMAKE_BUILD_TYPE=Debug -GXcode ..

cmake_minimum_required (VERSION 3.9)

set(VST3_SDK_ROOT "" CACHE PATH "Location of VST3 SDK")
set(public_sdk_SOURCE_DIR ${VST3_SDK_ROOT}/public.sdk)

project(MLVSTSample)

set(llllpluginnamellll_sources
    source/factoryDefinition.cpp
    source/pluginController.cpp
    source/pluginController.h
    source/pluginProcessor.cpp
    source/pluginProcessor.h
    source/version.h
)

set(target llllpluginnamellll)

SET(SMTG_ADD_VSTGUI OFF CACHE BOOL "Add VSTGUI support")
SET(VSTGUI_TOOLS OFF CACHE BOOL "Build VSTGUI Tools")

include(VST3_SDK.cmake)

smtg_add_vst3plugin(${target} ${llllpluginnamellll_sources})
set_target_properties(${target} PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
target_link_libraries(${target} PRIVATE base sdk)

if(SMTG_ADD_VSTGUI)
    target_include_directories(${target} PUBLIC ${VSTGUI_ROOT}/vstgui4)
    target_link_libraries(${target} PRIVATE base sdk vstgui_support)
endif()

if(SMTG_MAC)
    smtg_set_bundle(${target} INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/mac/Info.plist" PREPROCESS)
elseif(SMTG_WIN)
    target_sources(${target} PRIVATE resource/llllpluginnamellll.rc)
endif()


source_group(vstsdk REGULAR_EXPRESSION "vst3sdk+")


#---------------------------------------------------------
# add au target using auwrapper.

if(SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH)

	# TODO: VST3_OUTPUT_DIR should be declared somewhere more globally.
	set(VST3_OUTPUT_DIR ${CMAKE_BINARY_DIR}/VST3)

    set(target llllpluginnamellll_au)
    set(${target}_sources
        source/au/audiounitconfig.h
        source/au/Info.plist
    )
    add_library(${target} MODULE ${${target}_sources})
    smtg_setup_universal_binary(${target})
    set_target_properties(${target} PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
    set_target_properties(${target} PROPERTIES BUNDLE TRUE)
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GENERATE_MASTER_OBJECT_FILE "YES")
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_OTHER_LDFLAGS "-all_load")
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES")
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_WRAPPER_EXTENSION "component")
    set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${VST3_OUTPUT_DIR})
    target_link_libraries(${target} PRIVATE auwrapper)
    smtg_set_bundle(${target} INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/source/au/Info.plist" PREPROCESS)

    set(outputdir ${VST3_OUTPUT_DIR}/$<CONFIG>)

    add_dependencies(${target} llllpluginnamellll)
    add_custom_command(TARGET ${target} POST_BUILD 
        COMMAND /bin/mkdir "-p" ${outputdir}/${target}.component/Contents/Resources
        COMMAND /bin/rm "-rf" "${outputdir}/${target}.component/Contents/Resources/plugin.vst3"
        COMMAND ditto "-v" "${outputdir}/llllpluginnamellll.vst3" "${outputdir}/${target}.component/Contents/Resources/plugin.vst3"
	    COMMAND ditto "-v" "${outputdir}/${target}.component" "~/Library/Audio/Plug-Ins/Components/${target}.component"
    )

#    execute_process(COMMAND xcrun --find Rez OUTPUT_VARIABLE OSX_REZ_COMMAND OUTPUT_STRIP_TRAILING_WHITESPACE)
#    set(OSX_REZ_COMMAND "/usr/bin/Rez")
#    add_custom_command(TARGET ${target} POST_BUILD COMMAND "${OSX_REZ_COMMAND}"

    add_custom_command(TARGET ${target} POST_BUILD COMMAND "/usr/bin/Rez"
        "-d" "SystemSevenOrLater=1"
        "-script" "Roman"
        "-d" "i386_YES"
        "-d" "x86_64_YES"
        "-is" "${CMAKE_OSX_SYSROOT}"
        "-I" "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers"
        "-I" "/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers"
        "-I" "/System/Library/Frameworks/AudioUnit.framework/Versions/A/Headers/"
        "-I" "${SMTG_COREAUDIO_SDK_PATH}/AudioUnits/AUPublic/AUBase"
        "-I" "${CMAKE_CURRENT_LIST_DIR}/source/au"
        "-I" "${CMAKE_CURRENT_LIST_DIR}"
        "-o" "${outputdir}/${target}.component/Contents/Resources/llllpluginnamellll_au.rsrc"
        "-useDF"
         "${CMAKE_CURRENT_LIST_DIR}/source/auresource.r"
    )
endif()
