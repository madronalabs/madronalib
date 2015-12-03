//
//  MLClock.h
//  madronalib
//
//  Created by Randy Jones on 10/30/15.
//
//

#ifndef MLTimeUtils_h
#define MLTimeUtils_h

#include <chrono>
#include <cmath>
#include <iostream>

namespace ml {

// times and durations are stored in OSC / NTP timestamp format.
// This is a 32:32 fixed-point number, with 32 bits of seconds and 32 bits of fraction.
typedef uint64_t Time;
	
// conversion to double for general use. Note that floats do not have enough resolution 
// to store a time even accurate to the second.
double timeToDouble(Time ntpTime);
	
Time doubleToTime(double t);
Time samplesAtRateToTime(int samples, int rate);
	
class Clock
{
public:	
	static Time &theSystemTimeOffset()  { static TimeOffset t; return t.mOffset; }
	
	Clock();
	~Clock();
	
	Time now();
	
	void stop();
	void start();
	
	// adds the input time t to this Clock's offset. the DSP engine uses this to make a 
	// clock advance precisely in samples. 
	void advance(Time t);
	
private:
	class TimeOffset
	{
	public:
		TimeOffset();
		~TimeOffset();
		// offset from system time clock to steady clock, as measured on creation
		Time mOffset;
	};
	
	Time mOffset;
	bool mRunning;
	
};

	
} 

#endif
