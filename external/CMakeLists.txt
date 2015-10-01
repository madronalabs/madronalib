

if(BUILD_NEW_ONLY)
	add_subdirectory(cJSON)
else(BUILD_NEW_ONLY)
	add_subdirectory(cJSON)
	add_subdirectory(OSC)
	add_subdirectory(portaudio)
	add_subdirectory(juce)
endif()