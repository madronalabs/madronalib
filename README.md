Madronalib
----------

[![test](https://github.com/madronalabs/madronalib/actions/workflows/test.yaml/badge.svg)](https://github.com/madronalabs/madronalib/actions/workflows/test.yaml)

A C++ framework for DSP applications.

Copyright (c) 2019–2023 Madrona Labs LLC. http://www.madronalabs.com

Distributed under the MIT license: http://madrona-labs.mit-license.org/


C++ for audio: the good parts
------------

C++ is an excellent tool for writing audio software. Unfortunately it's difficult to get started with, and provides a mine field of bad ways to do things. Madronalib provides one clear path to writing maintainable audio code without sacrificing performance. It enables efficient audio DSP on SIMD processors with readable, brief code.

With the advent of C++11, new features became available as part of the standard library, and new syntax provided clearer ways to express abstractions with no overhead. Madronalib leans on modern C++ to reduce its code footprint and increase clarity.

To get reliable audio performance in a multiprocessing system, careful attention must be paid to memory allocation, shared object access, and any use of OS calls. (see: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) Madronalib encapsulates these concerns and provides a simple API to the writers of audio applications.

Adding new DSP modules is straightforward, and the syntax of the signal library encourages code to "read like the math." 

Madronalib comes with examples including VST3 synth and effect plugins (more formats to come). If you're wondering what these plugins look like, the answer is that they look like whatever generic interface your plugin host provides. The standalone examples look like console apps. Madronalib does not provide any kind of graphics capability. This helps enforce strict separation of concerns and enables what I like to call *audio-first development.*


Status
------------

(as of June 2024)

The files in /source/DSP are a useful header-only DSP library and can be included without other dependencies:  `#include mldsp.h`. These provide a bunch of utilities for writing efficient and readable DSP code in a functional style. SIMD operations for sin, cos, log and exp provide a big speed gain over native math libraries and come in both precise and approximate variations. Both SSE (for Intel chips) and NEON (for Apple Silicon) are supported. Shipping products at Madrona Labs are relying on these headers and breaking changes have, for the most part, stopped. 

There are three examples built using RtAudio that play and process audio signals. 

/examples/effect-plugin holds a simple VST3 effect plugin example. This plugin plays a couple of sine waves, and has no GUI. A script to clone plugin projects is included so you can make more interesting plugins. 

/examples/synth-plugin contains a VST3 synthesizer plugin example. 

Unlike the rest of madronalib, both the above examples require downloading the VST3 framework from Steinberg. They are not built as part of the main cmake build. Instructions for building these examples are in the individual directories.

The code in /source/app is also relied upon by shipping products, though it may still change more often. 

Tests exist for most modules. Very basic continuous integration exists using GitHub actions, but only tests on MacOS at present.

Building
----------

Madronalib can be built with the default settings as follows:

	mkdir build
	cd build
	cmake ..
	make
    
This will create a command-line build of all the new code.

To build and install with debug symbols enabled for use with the example plugin projects:

	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Debug ..
	make
	# Installation may require administrator account / sudo
	make install

To build an XCode project with JUCE support, run something like

	mkdir build-xcode
	cd build-xcode
	cmake -GXcode ..

To build a Visual Studio project for a 64-bit Windows app I'm currently using the command 

	cmake .. -G "Visual Studio 17 2022"


Contents
--------

	/cmake : any modules or tools for the cmake build system
  
	/examples: example projects
  
	/external: small supporting code projects included as source. cJSON, oscpack etc.
  
	/include: top-level public headers: mldsp.h, madronalib.h
  
	/source:
  
		/DSP: header-only library for SIMD DSP.
			These modules shall only depend on code in /DSP, 
			with the exception of the externals/ffft library. Include 
			mldsp.h to include all DSP files. 
      
		/app: utilities for low-level application support: symbols,
			timers, queues, data structures, models, etc. these modules shall 
			only depend on other code in /app.
      
		/matrix: matrix and vector data and functions. Depends only on 
			/DSP, for SIMD math.
      
		/networking: OSC and MIDI support.
    
		/procs: higher level DSP code designed to support dynamic graphs
			(changing at runtime.) These rely on code in /DSP as well as
			the symbolic code in /app. work in progress.
      
	/tests: tests for all modules implemented using the Catch library.




