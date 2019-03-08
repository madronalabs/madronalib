# madronalib/source/CMakeLists.txt

include_directories("${madronalib_SOURCE_DIR}/src"
                    "${madronalib_BINARY_DIR}/src"
                    ${madronalib_INCLUDE_DIRS})

#add_definitions(-D_MADRONALIB_USE_CONFIG_H)



set(madronalib_HEADERS
  ../include/madronalib.h
  )

set(OSC_SOURCES
    networking/MLInputProtocols.h
    networking/MLNetServiceHub.cpp
    networking/MLNetServiceHub.h
    networking/MLOSCListener.cpp
    networking/MLOSCListener.h
    networking/MLOSCReceiver.cpp
    networking/MLOSCReceiver.h
    networking/MLOSCSender.cpp
    networking/MLOSCSender.h
    networking/MLT3DHub.cpp
    networking/MLT3DHub.h
    networking/MLT3DPorts.h
)

set(NEW_SOURCES
    app/MLClock.cpp      
    app/MLPath.h         
    app/MLSymbol.cpp     
    app/MLTextUtils.cpp
    app/MLClock.h        
    app/MLPlatform.h     
    app/MLSymbol.h       
    app/MLTextUtils.h
    app/MLMemoryUtils.h  
    app/MLQueue.h        
    app/MLText.cpp       
    app/MLTimer.cpp
    app/MLPath.cpp       
    app/MLResourceMap.h  
    app/MLText.h         
    app/MLTimer.h
    app/MLModel.cpp
    app/MLModel.h
    app/MLProperty.cpp
    app/MLProperty.h
    app/MLPropertySet.cpp
    app/MLPropertySet.h

    DSP/MLDSPBuffer.cpp
    DSP/MLDSPBuffer.h
    DSP/MLDSPFilters.h
    DSP/MLDSPFunctional.h
    DSP/MLDSPGens.cpp
    DSP/MLDSPGens.h
    DSP/MLDSPMath.h
    DSP/MLDSPMathNEON.h
    DSP/MLDSPMathSSE.h
    DSP/MLDSPMathScalar.h
    DSP/MLDSPOps.h
    DSP/MLDSPUtils.h
    DSP/MLProjection.cpp
    DSP/MLProjection.h
    DSP/MLRatio.cpp
    DSP/MLRatio.h
    DSP/MLScalarConstants.h
    DSP/MLScalarMath.cpp
    DSP/MLScalarMath.h

    matrix/MLInterpolator.cpp
    matrix/MLInterpolator.h
    matrix/MLSignal.cpp
    matrix/MLSignal.h

    procs/MLProc.cpp
    procs/MLProc.h
    procs/MLProcFactory.cpp
    procs/MLProcFactory.h
    procs/MLProcMultiply.cpp
    procs/MLProcMultiply.h
    )

if(BUILD_NEW_ONLY)
   set(madronalib_SOURCES
    ${NEW_SOURCES}
    )

else(BUILD_NEW_ONLY)
    add_definitions(-D_MADRONALIB_TIMERS_USE_JUCE)
  set(madronalib_SOURCES
    ${NEW_SOURCES}
    MLProc.cpp
    deprecated/MLProc.h
    deprecated/MLProcFactory.cpp
    deprecated/MLProcFactory.h
    deprecated/MLProcMultiply.cpp
    deprecated/MLProcMultiply.h
    deprecated/MLChangeList.cpp
    deprecated/MLChangeList.h
    deprecated/MLControlEvent.cpp
    deprecated/MLControlEvent.h
    deprecated/MLDSPContext.cpp
    deprecated/MLDSPContext.h
    deprecated/MLDSPDeprecated.cpp
    deprecated/MLDSPDeprecated.h
    deprecated/MLDSPEngine.cpp
    deprecated/MLDSPEngine.h
    deprecated/MLDebug.cpp
    deprecated/MLDebug.h
    deprecated/MLDebugMac.mm
    deprecated/MLGL.cpp
    deprecated/MLGL.h
    deprecated/MLMultProxy.cpp
    deprecated/MLMultProxy.h
    deprecated/MLParameter.cpp
    deprecated/MLParameter.h
    deprecated/MLProc.cpp
    deprecated/MLProc.h
    deprecated/MLProcAbs.cpp
    deprecated/MLProcAdd.cpp
    deprecated/MLProcAllpass.cpp
    deprecated/MLProcBiquad.cpp
    deprecated/MLProcClamp.cpp
    deprecated/MLProcClampSignal.cpp
    deprecated/MLProcConstant.cpp
    deprecated/MLProcContainer.cpp
    deprecated/MLProcContainer.h
    deprecated/MLProcCubicDistort.cpp
    deprecated/MLProcDCBlocker.cpp
    deprecated/MLProcDebug.cpp
    deprecated/MLProcDelay.cpp
    deprecated/MLProcDelayInput.cpp
    deprecated/MLProcDelayInput.h
    deprecated/MLProcDelayOutput.cpp
    deprecated/MLProcDivide.cpp
    deprecated/MLProcDxDt.cpp
    deprecated/MLProcEnvelope.cpp
    deprecated/MLProcEnvelopeFollow.cpp
    deprecated/MLProcExp2.cpp
    deprecated/MLProcFMBandwidth.cpp
    deprecated/MLProcFade.cpp
    deprecated/MLProcFadeBipolar.cpp
    deprecated/MLProcGlide.cpp
    deprecated/MLProcHostPhasor.cpp
    deprecated/MLProcHostPhasor.h
    deprecated/MLProcInputToSignals.cpp
    deprecated/MLProcInputToSignals.h
    deprecated/MLProcInterleave.cpp
    deprecated/MLProcMatrix.cpp
    deprecated/MLProcMatrix.h
    deprecated/MLProcMultiple.cpp
    deprecated/MLProcMultiple.h
    deprecated/MLProcMultiply.cpp
    deprecated/MLProcMultiplyAdd.cpp
    deprecated/MLProcNoise.cpp
    deprecated/MLProcOnepole.cpp
    deprecated/MLProcPan.cpp
    deprecated/MLProcParamToSignal.cpp
    deprecated/MLProcPeak.cpp
    deprecated/MLProcPhasor.cpp
    deprecated/MLProcPow.cpp
    deprecated/MLProcQuantize.cpp
    deprecated/MLProcRMS.cpp
    deprecated/MLProcRMS.h
    deprecated/MLProcRate.cpp
    deprecated/MLProcResample.cpp
    deprecated/MLProcRingBuffer.cpp
    deprecated/MLProcRingBuffer.h
    deprecated/MLProcSampleRate.cpp
    deprecated/MLProcSineOsc.cpp
    deprecated/MLProcSinePhase.cpp
    deprecated/MLProcStereoPan.cpp
    deprecated/MLProcSubtract.cpp
    deprecated/MLProcSum.cpp
    deprecated/MLProcTestTone.cpp
    deprecated/MLProcThru.cpp
    deprecated/MLProcdBToAmp.cpp
    deprecated/MLSymbolMap.h
    deprecated/MLTextStreamListener.h
    deprecated/MLVectorDeprecated.cpp
    deprecated/MLVectorDeprecated.h

    JuceApp/MLAppBorder.cpp
    JuceApp/MLAppBorder.h
    JuceApp/MLAppState.cpp
    JuceApp/MLAppState.h
    JuceApp/MLAppView.cpp
    JuceApp/MLAppView.h
    JuceApp/MLAppWindow.cpp
    JuceApp/MLAppWindow.h
    JuceApp/MLAudioProcessorListener.h
    JuceApp/MLBoundsConstrainer.cpp
    JuceApp/MLBoundsConstrainer.h
    JuceApp/MLDefaultFileLocations.cpp
    JuceApp/MLDefaultFileLocations.h
    JuceApp/MLFile.cpp
    JuceApp/MLFile.h
    JuceApp/MLFileCollection.cpp
    JuceApp/MLFileCollection.h
    JuceApp/MLJuceAUWrapper.mm
    JuceApp/MLJuceFilesMac.cpp
    JuceApp/MLJuceFilesMac.h
    JuceApp/MLJuceInclude.h
    JuceApp/MLJuceOpenGL.h
    JuceApp/MLJuceVSTWrapper.cpp
    JuceApp/MLMenu.cpp
    JuceApp/MLMenu.h
    JuceApp/MLPageView.cpp
    JuceApp/MLPageView.h
    JuceApp/MLReporter.cpp
    JuceApp/MLReporter.h
    JuceApp/MLWidget.cpp
    JuceApp/MLWidget.h
    JuceApp/MLWidgetContainer.cpp
    JuceApp/MLWidgetContainer.h

    JuceLookAndFeel/MLButton.cpp
    JuceLookAndFeel/MLButton.h
    JuceLookAndFeel/MLDebugDisplay.cpp
    JuceLookAndFeel/MLDebugDisplay.h
    JuceLookAndFeel/MLDial.cpp
    JuceLookAndFeel/MLDial.h
    JuceLookAndFeel/MLDrawableButton.cpp
    JuceLookAndFeel/MLDrawableButton.h
    JuceLookAndFeel/MLDrawing.cpp
    JuceLookAndFeel/MLDrawing.h
    JuceLookAndFeel/MLEnvelope.cpp
    JuceLookAndFeel/MLEnvelope.h
    JuceLookAndFeel/MLImageBank.cpp
    JuceLookAndFeel/MLImageBank.h
    JuceLookAndFeel/MLLabel.cpp
    JuceLookAndFeel/MLLabel.h
    JuceLookAndFeel/MLLookAndFeel.cpp
    JuceLookAndFeel/MLLookAndFeel.h
    JuceLookAndFeel/MLMenuButton.cpp
    JuceLookAndFeel/MLMenuButton.h
    JuceLookAndFeel/MLMultiButton.cpp
    JuceLookAndFeel/MLMultiButton.h
    JuceLookAndFeel/MLMultiSlider.cpp
    JuceLookAndFeel/MLMultiSlider.h
    JuceLookAndFeel/MLPanel.cpp
    JuceLookAndFeel/MLPanel.h
    JuceLookAndFeel/MLPositioner.cpp
    JuceLookAndFeel/MLPositioner.h
    JuceLookAndFeel/MLProgressBar.cpp
    JuceLookAndFeel/MLProgressBar.h
    JuceLookAndFeel/MLSeparator.cpp
    JuceLookAndFeel/MLSeparator.h
    JuceLookAndFeel/MLTextButton.cpp
    JuceLookAndFeel/MLTextButton.h
    JuceLookAndFeel/MLToggleButton.cpp
    JuceLookAndFeel/MLToggleButton.h
    JuceLookAndFeel/MLTriToggleButton.cpp
    JuceLookAndFeel/MLTriToggleButton.h
    JuceLookAndFeel/MLUI.cpp
    JuceLookAndFeel/MLUI.h
    JuceLookAndFeel/MLUIBinaryData.cpp
    JuceLookAndFeel/MLUIBinaryData.h

    )
endif()

if(APPLE)
  list(APPEND madronalib_SOURCES deprecated/MLDebugMac.mm)
  list(APPEND madronalib_SOURCES OSC_SOURCES)
else()
  list(APPEND madronalib_SOURCES deprecated/MLDebug.cpp)
endif()


# Frameworks

#if(APPLE)
 #  FIND_LIBRARY(CARBON_LIBRARY Carbon)
 #  FIND_LIBRARY(QUICKTIME_LIBRARY QuickTime )
 #  FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
 #  SET(FRAMEWORKS ${CARBON_LIBRARY} ${QUICKTIME_LIBRARY} ${APP_SERVICES_LIBRARY})
#endif(APPLE)

# send binary output to the current build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set source groups

source_group(dsp REGULAR_EXPRESSION "DSP/.*")
source_group(app REGULAR_EXPRESSION "app/.*")
source_group(deprecated REGULAR_EXPRESSION "deprecated/.*")
source_group(JuceApp REGULAR_EXPRESSION "JuceApp/.*")
source_group(JuceLookAndFeel REGULAR_EXPRESSION "JuceLookAndFeel/.*")
source_group(matrix REGULAR_EXPRESSION "matrix/.*")
source_group(networking REGULAR_EXPRESSION "networking/.*")
source_group(procs REGULAR_EXPRESSION "procs/.*")

# Create Library


add_library(madronalib STATIC ${madronalib_SOURCES})
set_target_properties(madronalib PROPERTIES
                      OUTPUT_NAME "${madronalib_NAME}"
                      VERSION ${madronalib_VERSION}
                      SOVERSION ${madronalib_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
                      FOLDER "madronalib")


# Include Directories

target_include_directories(madronalib PUBLIC app)
target_include_directories(madronalib PUBLIC DSP)
target_include_directories(madronalib PUBLIC matrix)
target_include_directories(madronalib PUBLIC networking)
target_include_directories(madronalib PUBLIC procs)

if(BUILD_NEW_ONLY)
else(BUILD_NEW_ONLY)
  target_include_directories(madronalib PUBLIC deprecated)
  target_include_directories(madronalib PUBLIC JuceApp)
  target_include_directories(madronalib PUBLIC JuceLookAndFeel)  

  #include our AppConfig.h and JuceHeader.h
  target_include_directories(madronalib PUBLIC ../external/juce)
endif()


#--------------------------------------------------------------------
# Make docs (someday)
#--------------------------------------------------------------------


#if (DOXYGEN_FOUND AND ML_BUILD_DOCS)
#    add_subdirectory(docs)
#endif()


#--------------------------------------------------------------------
# Add libraries
#--------------------------------------------------------------------



target_link_libraries(madronalib aes256)
target_link_libraries(madronalib cjson)
# note: ffft and utf are header-only

if(APPLE)
    target_link_libraries(madronalib oscpack)
else()
endif()

# Juce Libraries

if(BUILD_NEW_ONLY)
else(BUILD_NEW_ONLY)
  target_link_libraries(madronalib juce_audio_basics)
  target_link_libraries(madronalib juce_audio_devices)
  target_link_libraries(madronalib juce_core)
  target_link_libraries(madronalib juce_graphics)
  target_link_libraries(madronalib juce_gui_basics)
  target_link_libraries(madronalib juce_gui_extra)
  target_link_libraries(madronalib juce_opengl)
endif()

if(APPLE)
  target_link_libraries(madronalib "-framework Carbon")
  target_link_libraries(madronalib "-framework CoreServices")
  target_link_libraries(madronalib "-framework AudioToolbox")
  target_link_libraries(madronalib "-framework AudioUnit")
  target_link_libraries(madronalib "-framework CoreAudio")
  target_link_libraries(madronalib "-framework Foundation")
  target_link_libraries(madronalib "-framework OpenGL")
  target_link_libraries(madronalib "-framework GLUT")
else(APPLE)
  #target_link_libraries(madronalib ${DNSSD_LIBRARIES})
endif()

if (BUILD_SHARED_LIBS)
    if (WIN32)
        # The MADRONALIB DLL needs a special compile-time macro and import library name
        set_target_properties(madronalib PROPERTIES PREFIX "" IMPORT_PREFIX "")

        if (MINGW)
            set_target_properties(madronalib PROPERTIES IMPORT_SUFFIX "dll.a")
        else()
            set_target_properties(madronalib PROPERTIES IMPORT_SUFFIX "dll.lib")
        endif()
    elseif (APPLE)
        # Append -fno-common to the compile flags to work around a bug in
        # Apple's GCC
        get_target_property(madronalib_CFLAGS madronalib COMPILE_FLAGS)
        if (NOT madronalib_CFLAGS)
            set(madronalib_CFLAGS "")
        endif()
        set_target_properties(madronalib PROPERTIES
                              COMPILE_FLAGS "${madronalib_CFLAGS} -fno-common"
                              INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
    endif()

    target_link_libraries(madronalib ${madronalib_LIBRARIES})
endif()
