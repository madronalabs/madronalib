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
	// A simple, low-resolution timer for doing applicaton and UI tasks.
	// Any callbacks are called synchronously from the main run loop so
	// callbacks should not take too much time. To trigger an action
	// that might take longer, send a message from the callback and
	// then receive it and do the action in a private thread.
	
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
		
		// call the function n times, waiting the specified interval before each.
		void callNTimes(std::function<void(void)> f, const milliseconds period, int n)
		{
			mCounter = n;
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

		bool isActive()
		{
			return mCounter != 0;
		}
		
		void stop();
		
	private:
		int mCounter{0};
		std::function<void(void)> myFunc;
		milliseconds mPeriod;
		time_point<system_clock> mPreviousCall;
	};
} // namespace ml


