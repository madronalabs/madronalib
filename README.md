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

Madronalib is designed to enable efficient audio DSP on SIMD processors with very readable code.

With the advent of C++11, new functionality became available as part of the standard library, and new syntax has provided clearer ways to express abstractions with no overhead. Madronalib leans on C++11 to reduce its code footprint and increase readability.

To get reliable audio performance in a multiprocessing system, careful attention must be paid to memory allocation, shared object access, and any use of OS calls. (see: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) Madronalib encapsulates these concerns and provides a simple API to the writers of audio applications.

Adding new DSP modules is straightforward. The syntax of the signal library encourages code to "read like the math." There are no weird macros to learn.

The Google C++ style guidelines (https://google-styleguide.googlecode.com/svn/trunk/cppguide.html) are followed, for the most part. 


Status
----------

Madronalib began as the common code supporting the Madrona Labs synthesizer products Aalto and Kaivo, as well as the open-source application for the Soundplane controller. All of this support code is currently present in the repository, but most is not part of the library build. Individual modules are being brought into the library as they are being reviewed, modernized and documented.

See the issues for planned functionality that is currently not present in any form.


Building
----------

Madronalib is currently used alongside the JUCE library to build the Madrona Labs plugins and the Soundplane application. Work is ongoing to remove the JUCE dependencies. For building only the new code independently of JUCE, use the BUILD_NEW_ONLY option in CMake as follows:

	mkdir build-new
	cd build-new
	cmake -DBUILD_NEW_ONLY=1 ..
	make

This will create a command-line build of all the new code with a test executable in 
madronalib/build-new-Tests. To add files to the new regime they must be added to 
madronalib_SOURCES; this list can be found in source/CMakeLists.txt.

To build an XCode project, run something like

	mkdir build-xcode
	cd build-xcode
	cmake -DBUILD_NEW_ONLY=1 -GXcode ..



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




