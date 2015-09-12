Madronalib
----------

A C++ framework for DSP applications.

Copyright (c) 2015 Madrona Labs LLC. http://www.madronalabs.com

Distributed under the MIT license: http://madrona-labs.mit-license.org/

Design Notes
------------

To get reliable audio performance in a multiprocessing system, careful attention must be paid to memory allocation, shared object access, and any use of OS calls. (see: http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) Madronalib encapsulates these concerns and provides a simple API to the writers of audio applications.

The most maintainable code is no code. With the advent of C++11, lots of functionality that previously required external libraries became available as part of C++. Madronalib leans on C++11 to reduce its code footprint.

Making new modules such as oscillators and filters is straightforward. The syntax of the signal library encourages code to reflect the underlying DSP equations with a minimum of overhead. There are no weird macros to learn.

Madronalib makes it possible to do DSP efficiently with very readable code. Where the goals of readability and efficiency are in conflict, madronalib favors readability.

The Google C++ style guidelines (https://google-styleguide.googlecode.com/svn/trunk/cppguide.html) are followed, for the most part. Any exceptions are well documented.


Status
----------

Madronalib began as the common code supporting the Madrona Labs synthesizer products Aalto and Kaivo, as well as the open-source application for the Soundplane controller. All of this support code is currently present in the repository, but most is not part of the library build. Individual modules are being brought into the library as they are being reviewed, modernized and documented.

See the issues for planned functionality that is currently not present in any form.


Building
----------

Madronalib is not typically meant to build on its own; it has to be built from within an
application that includes Madronalib's `CMakeLists.txt` and defines the variable
`ML_JUCE_HEADER_PATH` to be a path to a directory that contains a JUCE
`JuceHeader.h` and `AppConfig.h`.

Work is ongoing to remove the JUCE dependency from Madronalib. For building only the new 
code independently of JUCE, use the BUILD_NEW_ONLY option in CMake as follows:

	mkdir build-new
	cd build-new
	cmake -DBUILD_NEW_ONLY=1 ..
	make

This will create a command-line build of all the new code with a test executable in 
madronalib/build-new-Tests. To add files to the new regime they must be added to 
madronalib_SOURCES; this list can be found in source/CMakeLists.txt.



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





