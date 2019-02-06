Madronalib
----------

A C++ framework for DSP applications.

Copyright (c) 2015-2019 Madrona Labs LLC. http://www.madronalabs.com

Distributed under the MIT license: http://madrona-labs.mit-license.org/

Status
------------

As of Jan. 2019 a rewrite is in progress and some old code relied on by shipping products is still present. The examples and tests are the places to start looking at the new code. 

The files in /source/DSP are a useful standalone DSP library and can be included without other dependencies:  `#include MLDSP.h`. These provide a bunch of utilities for writing efficient and readable DSP code in a functional style. SIMD operations for sin, cos, log and exp provide a big speed gain over native math libraries and come in both precise and approximate variations.  An SSE version is complete, NEON support is a work in progress. 

Design Notes
------------

Madronalib is designed to enable efficient audio DSP on SIMD processors with readable, brief code.

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
	/include: MLDSP.h, madronalib.h
	/source:
		/core: utilities for low-level application support: symbols, 
			timers, queues, data structures, etc. these modules shall 
			only depend on other code in /core.

		/deprecated: code from the Juce-based app and the older dynamic 
			DSP graph code.

		/DSP: functions for making DSP objects including SSE/NEON math 
			libraries. These modules shall only depend on code in /DSP, 
			with the exception of the externals/ffft library. Include 
			MLDSP.h to include all DSP files. NEON code is work 
			in progress.

		/JuceApp: current code, adapters to JUCE framework.

		/JuceLookAndFeel: widgets for JUCE-based display.

		/matrix: matrix and vector data and functions. To depend only on 
			/DSP.

		/model: app framework code--properties, data, models, etc. 

		/networking: OSC and MIDI support.

		/procs: higher level DSP code designed to support dynamic graphs
			(changing at runtime.) These rely on code in /DSP as well as
			the symbolic code in /core. work in progress.

		/widgets where new UI widgets will go.

	/tests: tests for all new code.




