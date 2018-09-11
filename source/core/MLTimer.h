//
//  MLTimer.h
//  madronalib
//
//  Created by Randy Jones on 9/10/2018
//

#pragma once

#include "utf/utf.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

using namespace std::chrono;

namespace ml
{
	class Timer
	{
		friend class Timers;
	
	public:
		Timer() noexcept;
		~Timer();

		// delete copy and move constructors and assign operators
		Timer(Timer const&) = delete;             // Copy construct
		Timer(Timer&&) = delete;                  // Move construct
		Timer& operator=(Timer const&) = delete;  // Copy assign
		Timer& operator=(Timer &&) = delete;      // Move assign

		// call the function once after the specified interval.
		void callOnce(std::function<void(void)> f, const milliseconds period)
		{
			mCounter = 1;
			myFunc = f;
			mPeriod = period;
			mPreviousCall = system_clock::now();
		}
		
		// start calling the function periodically. the wait period happens before the first call.
		void start(std::function<void(void)> f, const milliseconds period)
		{
			mCounter = -1;
			myFunc = f;
			mPeriod = period;
			mPreviousCall = system_clock::now();
		}

		// stop
		void stop()
		{
			mCounter = 0;
		}

	private:
		int mCounter{0};
		std::function<void(void)> myFunc;
		milliseconds mPeriod;
		time_point<system_clock> mPreviousCall;
	};

} // namespace ml

