//
//  MLClock.cpp
//  madronalib
//
//  Created by Randy Jones on 10/31/15.
//
//

#include "MLClock.h"

namespace ml {

	double timeToDouble(Time ntpTime)
	{	
		static const double kScale = 1.0/pow(2., 32.);
		uint64_t hi = (ntpTime & 0xFFFFFFFF00000000) >> 32;
		uint64_t lo = ntpTime & 0xFFFFFFFF;
		double hiD = hi;
		double loD = static_cast<double>(lo)*kScale;
		return(hiD + loD);
	}
	
	Time doubleToTime(double t)
	{	
		static const double kLo32Scale = pow(2, 32);
		double floorT = floor(t);
		uint64_t hi32 = (static_cast<uint64_t>(floorT) & 0xFFFFFFFF) << 32;		
		double fractionalSecond = t - floorT;		
		uint64_t lo32 = static_cast<uint32_t>(fractionalSecond*kLo32Scale);
		return ((hi32 << 32) | lo32);	
	}

	Time samplesAtRateToTime(int samples, int rate)
	{	
		double t = static_cast<double>(samples)/static_cast<double>(rate);
		return doubleToTime(t);
	}
	
	
Clock::TimeOffset::TimeOffset()
{
	auto sysNow = std::chrono::system_clock::now();	
	auto sysMicros = std::chrono::time_point_cast<std::chrono::microseconds>(sysNow);
	uint64_t sysOffset = sysMicros.time_since_epoch().count();
	
	auto steadyNow = std::chrono::steady_clock::now();	
	auto steadyMicros = std::chrono::time_point_cast<std::chrono::microseconds>(steadyNow);
	uint64_t steadyOffset = steadyMicros.time_since_epoch().count();
	
	mOffset = sysOffset - steadyOffset;
}

Clock::TimeOffset::~TimeOffset() {}
	
Clock::Clock()
{
	// make local offset, and insure that the system time offset has been created
	mOffset = ml::Clock::theSystemTimeOffset();
}

Clock::~Clock()
{
	
}

uint64_t Clock::now()
{	
	if(mRunning)
	{
		static const double kLo32Scale = pow(2, 32)*0.000001;
		auto steadyNow = std::chrono::steady_clock::now();	
		auto steadyMicros = std::chrono::time_point_cast<std::chrono::microseconds>(steadyNow);
		uint64_t steadyOffset = steadyMicros.time_since_epoch().count();
		uint64_t steadyTimeInMicros = mOffset + steadyOffset;
		uint64_t steadyTimeInSeconds = steadyTimeInMicros*0.000001;		
		double fractionalSecondInMicros = (steadyTimeInMicros - steadyTimeInSeconds*1000000);		
		uint64_t hi32 = steadyTimeInSeconds;
		uint64_t lo32 = static_cast<uint32_t>(fractionalSecondInMicros*kLo32Scale);
		return ((hi32 << 32) | lo32);	
	}
	else
	{
		return mOffset;
	}
}
	
	void Clock::stop()
	{
		// is this good enough?
		if(mRunning)
		{
			mOffset = now();
		}
		mRunning = false;
	}
	
	void Clock::start()
	{
		mRunning = true;
	}
	
	void Clock::advance(Time t)
	{
		mOffset += t;
	}
	
}