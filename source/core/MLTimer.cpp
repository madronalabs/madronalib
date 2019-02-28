//
//  MLTimer.cpp
//  madronalib
//
//  Created by Randy Jones on 9/10/2018
//

// for use with the JUCE framework: https://github.com/WeAreROLI
// JUCE does not allow many display functions to be called from what it considers the wrong thread,
// so turn this on to start all our Timers from juce's message thread. 

#ifdef JUCE_APP_VERSION
#define MADRONALIB_TIMERS_USE_JUCE 1
#endif

#if MADRONALIB_TIMERS_USE_JUCE
#include "JuceHeader.h"
#endif

#include "MLTimer.h"

#include <set>
#include <mutex>

namespace ml
{
	namespace Time
	{
		const int kMillisecondsResolution = 10;
	}

	class Timers
#if MADRONALIB_TIMERS_USE_JUCE
	: private juce::Timer
#endif
	
	{
	public:
		
#if MADRONALIB_TIMERS_USE_JUCE
		Timers() { startTimer(Time::kMillisecondsResolution); }
		~Timers() { running = false; stopTimer(); }
#else
		Timers() { }
		~Timers() { running = false; runThread.join(); }
#endif
		
		// singleton: we only want one Timers instance. The first time a Timer object is made,
		// this object is made and the run thread is started.
		static Timers &theTimers()  { static Timers t; return t; }

		// delete copy and move constructors and assign operators
		Timers(Timers const&) = delete;             // Copy construct
		Timers(Timers&&) = delete;                  // Move construct
		Timers& operator=(Timers const&) = delete;  // Copy assign
		Timers& operator=(Timers &&) = delete;      // Move assign
		
		void insert(ml::Timer* t)
		{
			timerPtrs.insert(t);
		}
		
		void erase(ml::Timer* t)
		{
			timerPtrs.erase(t);
		}

		std::mutex mSetMutex;

	private:
		bool running { true };
		std::set< ml::Timer* > timerPtrs;

#if MADRONALIB_TIMERS_USE_JUCE
		void timerCallback()
		{
			runNow();
		}
#else
		 std::thread runThread { [&](){ run(); } };
#endif
		
		void runNow(void)
		{
			time_point<system_clock> now = system_clock::now();
			std::unique_lock<std::mutex> lock(mSetMutex);
			for(auto t : timerPtrs)
			{
				if(t->mCounter != 0)
				{
					if(now - t->mPreviousCall > t->mPeriod)
					{
						t->myFunc();
						if(t->mCounter > 0)
						{
							t->mCounter--;
						}
						t->mPreviousCall = now;
					}
				}
			}
		}
		
		void run(void)
		{
			while(running)
			{
				std::this_thread::sleep_for(milliseconds(Time::kMillisecondsResolution));
				runNow();
			}
		}
		
	}; // class Timers
	
	Timer::Timer() noexcept
	{
		std::unique_lock<std::mutex> lock(Timers::theTimers().mSetMutex);
		Timers::theTimers().insert(this);
	}
	
	Timer::~Timer()
	{
		std::unique_lock<std::mutex> lock(Timers::theTimers().mSetMutex);
		Timers::theTimers().erase(this);
	}
	
	void Timer::stop()
	{
		// More lightweight ways of handling a race on mCounter are possible, but as
		// stopping a timer is an infrequent operation we use the mutex for laziness and brevity.
		std::unique_lock<std::mutex> lock(Timers::theTimers().mSetMutex);
		mCounter = 0;
	}

} // namespace ml
