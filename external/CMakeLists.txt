

# CMakeLists.txt for madronalib/external 

add_subdirectory(cJSON)
add_subdirectory(OSC)
add_subdirectory(portaudio)	
target_include_directories(madronalib PUBLIC utf.hpp)	

if(!BUILD_NEW_ONLY)
	add_subdirectory(juce)
endif()