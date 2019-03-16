# madronalib/source/CMakeLists.txt

include_directories("${madronalib_SOURCE_DIR}/src"
                    "${madronalib_BINARY_DIR}/src"
                    ${madronalib_INCLUDE_DIRS})

#add_definitions(-D_MADRONALIB_USE_CONFIG_H)



set(madronalib_HEADERS
  ../include/madronalib.h
  )

set(NETWORKING_SOURCES
    networking/MLNetServiceHub.cpp
    networking/MLOSCListener.cpp
    networking/MLOSCReceiver.cpp
    networking/MLOSCSender.cpp
    networking/MLT3DHub.cpp
)

set(NETWORKING_HEADERS
    networking/MLInputProtocols.h
    networking/MLNetServiceHub.h
    networking/MLOSCListener.h
    networking/MLOSCReceiver.h
    networking/MLOSCSender.h
    networking/MLT3DHub.h
    networking/MLT3DPorts.h
)

set(DSP_HEADERS
    DSP/MLDSPBuffer.h
    DSP/MLDSPFilters.h
    DSP/MLDSPFunctional.h
    DSP/MLDSPGens.h
    DSP/MLDSPMath.h
    DSP/MLDSPMathNEON.h
    DSP/MLDSPMathSSE.h
    DSP/MLDSPMathScalar.h
    DSP/MLDSPOps.h
    DSP/MLDSPUtils.h
    DSP/MLProjection.h
    DSP/MLRatio.h
    DSP/MLScalarConstants.h
    DSP/MLScalarMath.h
)

set(app_HEADERS
    app/MLClock.h        
    app/MLMemoryUtils.h  
    app/MLModel.h
    app/MLPath.h         
    app/MLPlatform.h     
    app/MLProperty.h
    app/MLPropertySet.h
    app/MLQueue.h        
    app/MLResourceMap.h  
    app/MLSymbol.h       
    app/MLText.h         
    app/MLTextUtils.h
    app/MLTimer.h
)

set(matrix_HEADERS
    matrix/MLInterpolator.h
    matrix/MLSignal.h
)

set(proc_HEADERS
    procs/MLProc.h
    procs/MLProcFactory.h
    procs/MLProcMultiply.h
)

set(madronalib_SOURCES
    ${DSP_HEADERS}
    ${app_HEADERS}
    ${matrix_HEADERS}
    ${proc_HEADERS}
    
    app/MLClock.cpp      
    app/MLModel.cpp
    app/MLPath.cpp       
    app/MLProperty.cpp
    app/MLPropertySet.cpp
    app/MLSymbol.cpp     
    app/MLTextUtils.cpp
    app/MLText.cpp       
    app/MLTimer.cpp

    matrix/MLInterpolator.cpp
    matrix/MLSignal.cpp

    procs/MLProc.cpp
    procs/MLProcFactory.cpp
    procs/MLProcMultiply.cpp
    )

# TODO ml-juce Timer integration
#    add_definitions(-D_MADRONALIB_TIMERS_USE_JUCE)



if(APPLE)
  list(APPEND madronalib_SOURCES ${NETWORKING_SOURCES} )
else()
  #TODO OSC for windows
endif()


# Frameworks


# send binary output to the current build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#--------------------------------------------------------------------
# set source groups for source above
#--------------------------------------------------------------------

source_group(dsp REGULAR_EXPRESSION "DSP/.*")
source_group(app REGULAR_EXPRESSION "app/.*")
source_group(deprecated REGULAR_EXPRESSION "deprecated/.*")
source_group(JuceApp REGULAR_EXPRESSION "JuceApp/.*")
source_group(JuceLookAndFeel REGULAR_EXPRESSION "JuceLookAndFeel/.*")
source_group(matrix REGULAR_EXPRESSION "matrix/.*")
source_group(networking REGULAR_EXPRESSION "networking/.*")
source_group(procs REGULAR_EXPRESSION "procs/.*")

#--------------------------------------------------------------------
# create and install libraries
#--------------------------------------------------------------------


add_library(madronalib STATIC ${madronalib_SOURCES})
set_target_properties(madronalib PROPERTIES
                      OUTPUT_NAME "${madronalib_NAME}"
                      VERSION ${madronalib_VERSION}
                      SOVERSION ${madronalib_VERSION_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
                      FOLDER "madronalib")

include(GNUInstallDirs)

install(
    TARGETS madronalib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ../include/mldsp.h DESTINATION include/madronalib)

install(FILES ${DSP_HEADERS} DESTINATION include/madronalib)

install(FILES ../external/cJSON/cJSON.h DESTINATION include/madronalib)

install(FILES ../include/madronalib.h DESTINATION include/madronalib)

install(FILES 
    ${app_HEADERS}     
    ${matrix_HEADERS}
    ${proc_HEADERS} 
    DESTINATION include/madronalib)

if(APPLE)
    install(FILES ${NETWORKING_HEADERS} DESTINATION include/madronalib)
else()
    #TODO OSC For win / Linux
endif()


#--------------------------------------------------------------------
# Add include Directories
#--------------------------------------------------------------------

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
