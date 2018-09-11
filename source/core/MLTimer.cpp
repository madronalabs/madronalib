//
//  MLTimer.cpp
//  madronalib
//
//  Created by Randy Jones on 9/10/2018
//

#include "MLTimer.h"

#include <set>

namespace ml
{
	namespace Time
	{
		const int kMillisecondsResolution = 10;
	}

	class Timers
	{
	public:
		Timers() { }
		~Timers() { running = false; runThread.join(); }
		
		// singleton: we only want one Timers instance.
		static Timers &theTimers()  { static Timers t; return t; }

		// delete copy and move constructors and assign operators
		Timers(Timers const&) = delete;             // Copy construct
		Timers(Timers&&) = delete;                  // Move construct
		Timers& operator=(Timers const&) = delete;  // Copy assign
		Timers& operator=(Timers &&) = delete;      // Move assign
		
		void insert(Timer* t)
		{
			timerPtrs.insert(t);
		}
		
		void erase(Timer* t)
		{
			timerPtrs.erase(t);
		}

		std::mutex mSetMutex;

	private:
		bool running { true };
		std::set< Timer* > timerPtrs;
		std::thread runThread { [&](){ run(); } };
		
		void run(void)
		{
			while(running)
			{
				std::this_thread::sleep_for(milliseconds(Time::kMillisecondsResolution));
				
				time_point<system_clock> now = system_clock::now();
				{
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
			}
		}
	};
	
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

} // namespace ml
