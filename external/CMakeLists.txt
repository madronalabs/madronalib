
add_subdirectory(cJSON)
add_subdirectory(OSC)
add_subdirectory(portaudio)

if(BUILD_NEW_ONLY)
else(BUILD_NEW_ONLY)
	add_subdirectory(juce)
endif()