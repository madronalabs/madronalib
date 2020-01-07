# root CMakeLists.txt for vst3-again-sample built out of source.
# to run, set VST3_SDK_ROOT to inform CMake where the VST3 SDK is, for example:
# mkdir build
# cd build
# cmake -DVST3_SDK_ROOT=/Users/randy/dev/vst3sdk -DCMAKE_BUILD_TYPE=Debug -GXcode ..
#

cmake_minimum_required (VERSION 3.9)


set(VST3_SDK_ROOT "" CACHE PATH "Location of VST3 SDK")
set(public_sdk_SOURCE_DIR ${VST3_SDK_ROOT}/public.sdk)

project(MLVSTSample)

include(VST3_SDK.cmake)


set(mda-vst3_sources
    source/helpers.h

    source/mdaBaseController.cpp
    source/mdaBaseController.h
    source/mdaBaseProcessor.cpp
    source/mdaBaseProcessor.h


    source/mdafactory.cpp
    
    source/mdaParameter.cpp
    source/mdaParameter.h

    source/mdaTrackerController.cpp
    source/mdaTrackerController.h
    source/mdaTrackerProcessor.cpp
    source/mdaTrackerProcessor.h
    source/version.h
)

set(target mda-vst3)
smtg_add_vst3plugin(${target} ${mda-vst3_sources})
set_target_properties(${target} PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
target_link_libraries(${target} PRIVATE base sdk)
if(SMTG_MAC)
    smtg_set_bundle(${target} INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/mac/Info.plist" PREPROCESS)
elseif(SMTG_WIN)
    target_sources(${target} PRIVATE resource/mda-vst3.rc)
endif()












#
#
#
#set(again_sources
#	source/again.cpp
#	source/again.h
#	source/againcids.h
#	source/againcontroller.cpp
#	source/againcontroller.h
#	source/againentry.cpp
#	source/againparamids.h
#	source/againprocess.h
#	source/againuimessagecontroller.h
#	source/version.h
#)
#
#set(target again)
#smtg_add_vst3plugin(${target} ${again_sources})
#set_target_properties(${target} PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
#target_include_directories(${target} PUBLIC ${VSTGUI_ROOT}/vstgui4)
#target_link_libraries(${target} PRIVATE base sdk vstgui_support)
#
#smtg_add_vst3_resource(${target} "resource/again.uidesc")
#smtg_add_vst3_resource(${target} "resource/background.png")
#smtg_add_vst3_resource(${target} "resource/slider_background.png")
#smtg_add_vst3_resource(${target} "resource/slider_handle.png")
#smtg_add_vst3_resource(${target} "resource/slider_handle_2.0x.png")
#smtg_add_vst3_resource(${target} "resource/vu_on.png")
#smtg_add_vst3_resource(${target} "resource/vu_off.png")
#
#if(MAC)
#	smtg_set_bundle(${target} INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/mac/Info.plist" PREPROCESS)
#elseif(WIN)
#	target_sources(${target} PRIVATE resource/again.rc)
#endif()
#


#---------------------------------------------------------
# au target using auwrapper.
#
#if(SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH AND SMTG_ADD_VSTGUI)
#	# TODO: VST3_OUTPUT_DIR should be declared somewhere more globally.
#	set(VST3_OUTPUT_DIR ${CMAKE_BINARY_DIR}/VST3)
#
#    set(target again_au)
#    set(${target}_sources
#        source/au/doc.cpp
#        source/au/audiounitconfig.h
#        source/au/Info.plist
#    )
#    add_library(${target} MODULE ${${target}_sources})
#    smtg_setup_universal_binary(${target})
#    set_target_properties(${target} PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
#    set_target_properties(${target} PROPERTIES BUNDLE TRUE)
#    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GENERATE_MASTER_OBJECT_FILE "YES")
#    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_OTHER_LDFLAGS "-all_load")
#    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES")
#    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_WRAPPER_EXTENSION "component")
#    set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${VST3_OUTPUT_DIR})
#    target_link_libraries(${target} PRIVATE auwrapper)
#    smtg_set_bundle(${target} INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/source/au/Info.plist" PREPROCESS)
#
#    set(outputdir ${VST3_OUTPUT_DIR}/$<CONFIG>)
#
#    add_dependencies(${target} again)
#    add_custom_command(TARGET ${target} POST_BUILD 
#        COMMAND /bin/mkdir "-p" ${outputdir}/${target}.component/Contents/Resources
#        COMMAND /bin/rm "-rf" "${outputdir}/${target}.component/Contents/Resources/plugin.vst3"
#        COMMAND ditto "-v" "${outputdir}/again.vst3" "${outputdir}/${target}.component/Contents/Resources/plugin.vst3"
#	    COMMAND ditto "-v" "${outputdir}/${target}.component" "~/Library/Audio/Plug-Ins/Components/${target}.component"
#    )
#
#    execute_process(COMMAND xcrun --find Rez OUTPUT_VARIABLE OSX_REZ_COMMAND OUTPUT_STRIP_TRAILING_WHITESPACE)
#    add_custom_command(TARGET ${target} POST_BUILD COMMAND "${OSX_REZ_COMMAND}"
#        "-d" "SystemSevenOrLater=1"
#        "-script" "Roman"
#        "-d" "i386_YES"
#        "-d" "x86_64_YES"
#        "-is" "${CMAKE_OSX_SYSROOT}"
#        "-I" "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/#A/Headers"
#        "-I" "/System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers"
#        "-I" "/System/Library/Frameworks/AudioUnit.framework/Versions/A/Headers/"
#        "-I" "${SMTG_COREAUDIO_SDK_PATH}/AudioUnits/AUPublic/AUBase"
#        "-I" "${CMAKE_CURRENT_LIST_DIR}/source/au"
#        "-I" "${CMAKE_CURRENT_LIST_DIR}"
#        "-o" "${outputdir}/${target}.component/Contents/Resources/again_au.rsrc"
#        "-useDF"
#         "${CMAKE_CURRENT_LIST_DIR}/source/auresource.r"
#    )
#
#endif()
#
#
#
#