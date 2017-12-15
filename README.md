Madronalib
----------

A C++ framework for DSP applications.

Copyright (c) 2015 Madrona Labs LLC. http://www.madronalabs.com

Distributed under the MIT license: http://madrona-labs.mit-license.org/

Status
------------

As of Jan. 2017 things are still in a lot of flux. A rewrite is in progress and the old version, relied on in parts by shipping products, is still present. The examples and tests are the places to start looking at the new code. 

Design Notes
------------

Madronalib is designed to enable efficient audio DSP on SIMD processors with readable code.

With the advent of C++11, new functionality became available as part of the standard library, and new syntax has provided clearer ways to express abstractions with no overhead. Madronalib leans on C++11 to reduce its code footprint and increase clarity.

To get reliable audio performance in a multiprocessing system, careful attention must be paid to memory allocation, shared object access, and any use of OS calls. (see: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) Madronalib encapsulates these concerns and provides a simple API to the writers of audio applications.

Adding new DSP modules is straightforward. The syntax of the signal library encourages code to "read like the math." There are no weird macros to learn.

The Google C++ style guidelines (https://google-styleguide.googlecode.com/svn/trunk/cppguide.html) are followed, for the most part. 


Building
----------

Madronalib is currently used alongside the JUCE library to build the Madrona Labs plugins and the Soundplane application. Work is ongoing to remove the dual-licensed JUCE dependencies. The default build does not include these modules. For building only the new code independently of JUCE, use the default settings as follows:

	mkdir build
	cd build
	cmake ..
	make
    
This will create a command-line build of all the new code.

To build with JUCE app support, first get the JUCE submodule then turn the BUILD_NEW_ONLY option off in CMake as follows:

    git submodule update --init --recursive
    mkdir build
    cd build
    cmake -DBUILD_NEW_ONLY=OFF ..
    make


To build an XCode project with JUCE support, run something like

	mkdir build-xcode
	cd build-xcode
	cmake -DBUILD_NEW_ONLY=OFF -GXcode ..



Contents
--------

	/cmake : any modules or tools for the cmake build system
	/examples: example projects
	/external: small supporting code projects included in their entirety. TinyXML, cJSON, portaudio etc. 
	/include: madronalib.h
	/source:
		/core: new files with tests and doxygen documentation, suitable for use outside the Labs. 
			files in here must not be dependent on JUCE. 
			files are being moved in as they are cleaned up.
			all files in core will be included in /include/madronalib.h as they are moved in.

		/DSP: some DSP utils to move to /core. many procs to move to /procs.
		/LookAndFeel: widgets for JUCE-based display, to convert to OpenGL widgets.
		/MLApp: current code, MVC framework for apps. to move to /core.
		/MLJuceApp: current code, adapters to JUCE framework, to remove.
		/procs: where converted DSP processors will go. 
		/widgets where new widgets will go.
	/tests




