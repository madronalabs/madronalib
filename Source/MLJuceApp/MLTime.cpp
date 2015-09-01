
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLTime.h"

#include "JuceHeader.h"
// #include "modules/juce_core/time/juce_Time.h"

int64 getMicroseconds()
{

/*
#if ML_WINDOWS
	UInt64 millis = timeGetTime();
	r = millis*1000;
#elif ML_LINUX
	timespec t;
    clock_gettime (CLOCK_MONOTONIC, &t);
    r = t.tv_sec * 1000000 + t.tv_nsec / 1000;
#elif ML_MAC
	UnsignedWide micros;
    Microseconds(&micros);
    UInt64 microsHi = micros.hi;
    r = (microsHi << 32) + micros.lo;
#endif

*/

	int64 now = juce::Time::getHighResolutionTicks();
	double t = juce::Time::highResolutionTicksToSeconds (now);
	double micros = t*1000000.;
	int64 m = micros;
	return m;
}


