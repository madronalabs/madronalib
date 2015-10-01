# madronalib/source/CMakeLists.txt

include_directories("${madronalib_SOURCE_DIR}/src"
                    "${madronalib_BINARY_DIR}/src"
                    ${madronalib_INCLUDE_DIRS})

#add_definitions(-D_MADRONALIB_USE_CONFIG_H)


set(madronalib_HEADERS
  ../include/madronalib.h
  )

if(BUILD_NEW_ONLY)
   set(madronalib_SOURCES
    core/MLDSP.cpp
    core/MLDSP.h
    core/MLLocks.h
    core/MLSignal.cpp
    core/MLSignal.h
    core/MLSymbol.cpp
    core/MLSymbol.h
    core/MLStringCompare.h
    core/MLStringUtils.cpp
    core/MLStringUtils.h
    core/MLResourceMap.h
    core/MLVector.cpp
    core/MLVector.h
    )
else(BUILD_NEW_ONLY)
  
  if(NOT APPLE)
    find_package(GLUT REQUIRED)
    find_package(DNSSD REQUIRED)
  endif()

  set(madronalib_SOURCES
    core/MLDSP.cpp
    core/MLDSP.h
    core/MLLocks.h
    core/MLSignal.cpp
    core/MLSignal.h
    core/MLSymbol.cpp
    core/MLSymbol.h
    core/MLStringCompare.h
    core/MLStringUtils.cpp
    core/MLStringUtils.h
    core/MLResourceMap.h
    core/MLVector.cpp
    core/MLVector.h
    DSP/MLChangeList.cpp
    DSP/MLChangeList.h
    DSP/MLControlEvent.cpp
    DSP/MLControlEvent.h
    DSP/MLDSPContext.cpp
    DSP/MLDSPContext.h
    DSP/MLDSPEngine.cpp
    DSP/MLDSPEngine.h
    DSP/MLDSPUtils.cpp
    DSP/MLDSPUtils.h
    DSP/MLFFT.cpp
    DSP/MLFFT.h
    DSP/MLMultProxy.cpp
    DSP/MLMultProxy.h
    DSP/MLParameter.cpp
    DSP/MLParameter.h
    DSP/MLProc.cpp
    DSP/MLProc.h
    DSP/MLProcAbs.cpp
    DSP/MLProcAdd.cpp
    DSP/MLProcAllpass.cpp
    DSP/MLProcBiquad.cpp
    DSP/MLProcClamp.cpp
    DSP/MLProcClampSignal.cpp
    DSP/MLProcContainer.cpp
    DSP/MLProcContainer.h
    DSP/MLProcCubicDistort.cpp
    DSP/MLProcDCBlocker.cpp
    DSP/MLProcDebug.cpp
    DSP/MLProcDelay.cpp
    DSP/MLProcDelayInput.cpp
    DSP/MLProcDelayInput.h
    DSP/MLProcDelayOutput.cpp
    DSP/MLProcDivide.cpp
    DSP/MLProcDxDt.cpp
    DSP/MLProcEnvelope.cpp
    DSP/MLProcExp2.cpp
    DSP/MLProcFade.cpp
    DSP/MLProcFadeBipolar.cpp
    DSP/MLProcFMBandwidth.cpp
    DSP/MLProcGlide.cpp
    DSP/MLProcHostPhasor.cpp
    DSP/MLProcHostPhasor.h
    DSP/MLProcInputToSignals.cpp
    DSP/MLProcInputToSignals.h
    DSP/MLProcInterleave.cpp
    DSP/MLProcMatrix.cpp
    DSP/MLProcMatrix.h
    DSP/MLProcMultiple.cpp
    DSP/MLProcMultiple.h
    DSP/MLProcMultiply.cpp
    DSP/MLProcMultiplyAdd.cpp
    DSP/MLProcNoise.cpp
    DSP/MLProcOnepole.cpp
    DSP/MLProcPan.cpp
    DSP/MLProcParamToSignal.cpp
    DSP/MLProcPeak.cpp
    DSP/MLProcPhasor.cpp
    DSP/MLProcPow.cpp
    DSP/MLProcQuantize.cpp
    DSP/MLProcRate.cpp
    DSP/MLProcResample.cpp
    DSP/MLProcRingBuffer.cpp
    DSP/MLProcRingBuffer.h
    DSP/MLProcRMS.cpp
    DSP/MLProcRMS.h
    DSP/MLProcSampleRate.cpp
    DSP/MLProcSineOsc.cpp
    DSP/MLProcSinePhase.cpp
    DSP/MLProcSubtract.cpp
    DSP/MLProcSum.cpp
    DSP/MLProcSVF.cpp
    DSP/MLProcTestTone.cpp
    DSP/MLProcThru.cpp
    DSP/MLProperty.cpp
    DSP/MLProperty.h
    DSP/MLRatio.cpp
    DSP/MLRatio.h
    DSP/MLRingBuffer.cpp
    DSP/MLRingBuffer.h
    DSP/MLScale.cpp
    DSP/MLScale.h
    LookAndFeel/MLButton.cpp
    LookAndFeel/MLButton.h
    LookAndFeel/MLDebugDisplay.cpp
    LookAndFeel/MLDebugDisplay.h
    LookAndFeel/MLDial.cpp
    LookAndFeel/MLDial.h
    LookAndFeel/MLDrawableButton.cpp
    LookAndFeel/MLDrawableButton.h
    LookAndFeel/MLDrawing.cpp
    LookAndFeel/MLDrawing.h
    LookAndFeel/MLEnvelope.cpp
    LookAndFeel/MLEnvelope.h
    LookAndFeel/MLImageBank.cpp
    LookAndFeel/MLImageBank.h
    LookAndFeel/MLLabel.cpp
    LookAndFeel/MLLabel.h
    LookAndFeel/MLLookAndFeel.cpp
    LookAndFeel/MLLookAndFeel.h
    LookAndFeel/MLMenuButton.cpp
    LookAndFeel/MLMenuButton.h
    LookAndFeel/MLMultiButton.cpp
    LookAndFeel/MLMultiButton.h
    LookAndFeel/MLMultiSlider.cpp
    LookAndFeel/MLMultiSlider.h
    LookAndFeel/MLPanel.cpp
    LookAndFeel/MLPanel.h
    LookAndFeel/MLPolynomialWidget.cpp
    LookAndFeel/MLPolynomialWidget.h
    LookAndFeel/MLPositioner.cpp
    LookAndFeel/MLPositioner.h
    LookAndFeel/MLProgressBar.cpp
    LookAndFeel/MLProgressBar.h
    LookAndFeel/MLSeparator.cpp
    LookAndFeel/MLSeparator.h
    LookAndFeel/MLTextButton.cpp
    LookAndFeel/MLTextButton.h
    LookAndFeel/MLToggleButton.cpp
    LookAndFeel/MLToggleButton.h
    LookAndFeel/MLTriToggleButton.cpp
    LookAndFeel/MLTriToggleButton.h
    LookAndFeel/MLUI.cpp
    LookAndFeel/MLUI.h
    LookAndFeel/MLUIBinaryData.cpp
    LookAndFeel/MLUIBinaryData.h
    LookAndFeel/UIData/MLUIBinaryData/MLUIBinaryData.cpp
    LookAndFeel/UIData/MLUIBinaryData/MLUIBinaryData.h
    MLApp/MLDebug.cpp
    MLApp/MLDebug.h
    MLApp/MLGL.cpp
    MLApp/MLGL.h
    MLApp/MLInputProtocols.h
    MLApp/MLModel.cpp
    MLApp/MLModel.h
    MLApp/MLNetServiceHub.cpp
    MLApp/MLNetServiceHub.h
    MLApp/MLOSCListener.cpp
    MLApp/MLOSCListener.h
    MLApp/MLPath.cpp
    MLApp/MLPath.h
    MLApp/MLPlatform.h
    MLApp/MLReporter.cpp
    MLApp/MLReporter.h
    MLApp/MLSymbolMap.h
    MLApp/MLT3DHub.cpp
    MLApp/MLT3DHub.h
    MLApp/MLTextStreamListener.h
    MLJuceApp/MLAppBorder.cpp
    MLJuceApp/MLAppBorder.h
    MLJuceApp/MLAppState.cpp
    MLJuceApp/MLAppState.h
    MLJuceApp/MLAppView.cpp
    MLJuceApp/MLAppView.h
    MLJuceApp/MLAppWindow.cpp
    MLJuceApp/MLAppWindow.h
    MLJuceApp/MLAudioProcessorListener.h
    MLJuceApp/MLBoundsConstrainer.cpp
    MLJuceApp/MLBoundsConstrainer.h
    MLJuceApp/MLDefaultFileLocations.cpp
    MLJuceApp/MLDefaultFileLocations.h
    MLJuceApp/MLFile.cpp
    MLJuceApp/MLFile.h
    MLJuceApp/MLFileCollection.cpp
    MLJuceApp/MLFileCollection.h
    MLJuceApp/MLJuceFilesMac.cpp
    MLJuceApp/MLJuceFilesMac.h
    MLJuceApp/MLJuceInclude.h
    MLJuceApp/MLJuceOpenGL.h
    MLJuceApp/MLMenu.cpp
    MLJuceApp/MLMenu.h
    MLJuceApp/MLPageView.cpp
    MLJuceApp/MLPageView.h
    MLJuceApp/MLPluginController.cpp
    MLJuceApp/MLPluginController.h
    MLJuceApp/MLPluginFormats.h
    MLJuceApp/MLPluginView.cpp
    MLJuceApp/MLPluginView.h
    MLJuceApp/MLSignalReporter.cpp
    MLJuceApp/MLSignalReporter.h
    MLJuceApp/MLSignalView.cpp
    MLJuceApp/MLSignalView.h
    MLJuceApp/MLTime.cpp
    MLJuceApp/MLTime.h
    MLJuceApp/MLWidget.cpp
    MLJuceApp/MLWidget.h
    MLJuceApp/MLWidgetContainer.cpp
    MLJuceApp/MLWidgetContainer.h
    )
endif()

if(APPLE)
  list(APPEND madronalib_SOURCES MLApp/MLDebugMac.mm)
else(APPLE)
  list(APPEND madronalib_SOURCES MLApp/MLDebug.cpp)
endif()

# send binary output to the current build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_library(madronalib ${madronalib_HEADERS} ${madronalib_SOURCES})
set_target_properties(madronalib PROPERTIES
                      OUTPUT_NAME "${madronalib_NAME}"
                      VERSION ${madronalib_VERSION}
                      SOVERSION ${madronalib_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
                      FOLDER "madronalib")

if(BUILD_NEW_ONLY)
  target_include_directories(madronalib PUBLIC core)
else(BUILD_NEW_ONLY)
  target_include_directories(madronalib PUBLIC core)
  target_include_directories(madronalib PUBLIC MLJuceApp)
  target_include_directories(madronalib PUBLIC MLApp)
  target_include_directories(madronalib PUBLIC DSP)
  target_include_directories(madronalib PUBLIC LookAndFeel)
endif()

if(BUILD_NEW_ONLY)
  target_link_libraries(madronalib cjson)
else(BUILD_NEW_ONLY)
  target_link_libraries(madronalib portaudio)
  target_link_libraries(madronalib oscpack)
  target_link_libraries(madronalib cjson)
  target_link_libraries(madronalib juce_audio_basics)
  target_link_libraries(madronalib juce_audio_devices)
  target_link_libraries(madronalib juce_core)
  target_link_libraries(madronalib juce_graphics)
  target_link_libraries(madronalib juce_gui_basics)
  target_link_libraries(madronalib juce_gui_extra)
  target_link_libraries(madronalib juce_opengl)
endif()

if(APPLE)
  target_link_libraries(madronalib "-framework Foundation")
  target_link_libraries(madronalib "-framework OpenGL")
  target_link_libraries(madronalib "-framework GLUT")
else(APPLE)
  target_link_libraries(madronalib ${DNSSD_LIBRARIES})
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
