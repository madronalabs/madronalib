//
//  MLTimer.cpp
//  madronalib
//
//  Created by Randy Jones on 9/10/2018
//

#include <set>
#include <mutex>

#include "MLTimer.h"

namespace ml
{
  // Timers

  const int Timers::kMillisecondsResolution = 16;

  void Timers::tick(void)
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

  void Timers::run(void)
  {
    running = true; 
    while(running)
    {
      std::this_thread::sleep_for(milliseconds(Timers::kMillisecondsResolution));
      tick();
    }
  }
	
  // Timer

  Timer::Timer() noexcept
	{
    Timers& t{Timers::theTimers()};
		std::unique_lock<std::mutex> lock(t.mSetMutex);
		t.insert(this);
	}
	
	Timer::~Timer()
	{
    Timers& t{Timers::theTimers()};
		std::unique_lock<std::mutex> lock(t.mSetMutex);
		t.erase(this);
	}
	
	void Timer::stop()
	{
		// More lightweight ways of handling a race on mCounter are possible, but as
		// stopping a timer is an infrequent operation we use the mutex for laziness and brevity.
    Timers& t{Timers::theTimers()};
		std::unique_lock<std::mutex> lock(t.mSetMutex);
		mCounter = 0;
	}
} // namespace ml
