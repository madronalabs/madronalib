//
//  MLTimer.h
//  madronalib
//
//  Created by Randy Jones on 9/10/2018
//

#pragma once

#include <mutex>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <set>

using namespace std::chrono;

namespace ml
{
	// A simple, low-resolution timer for doing applicaton and UI tasks.
	// Any callbacks are called synchronously from the main run loop so
	// callbacks should not take too much time. To trigger an action
	// that might take longer, send a message from the callback and
	// then receive it and do the action in a private thread.

  class Timer;

  class Timers
  {
  public:
    static const int kMillisecondsResolution;
    static void* pTimersRef;
    
    Timers() { }
    ~Timers() { if(running) stop(); }

    // singleton: we only want one Timers instance.
    static Timers &theTimers()  { static Timers t; return t; }
    // delete copy and move constructors and assign operators
    Timers(Timers const&) = delete;             // Copy construct
    Timers(Timers&&) = delete;                  // Move construct
    Timers& operator=(Timers const&) = delete;  // Copy assign
    Timers& operator=(Timers &&) = delete;      // Move assign

    // To start it running, call start() on the single Timers
    // instance. If runInMainThread is true, the timers will
    // be called from the application's main thread, on operating
    // systems that have this concept.
    void start(bool runInMainThread = false);
    void stop();
    
    void insert(Timer* t)
    {
      timerPtrs.insert(t);
    }

    void erase(Timer* t)
    {
      timerPtrs.erase(t);
    }

    void tick(void);
    void run(void);


    std::mutex mSetMutex;

  private:
    bool running { false };
    bool inMainThread { false };
    std::set< Timer* > timerPtrs;
    std::thread runThread;
//    void* pImpl{nullptr};
    
    
  }; // class Timers

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


