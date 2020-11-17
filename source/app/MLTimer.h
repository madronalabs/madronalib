//
//  MLTimer.h
//  madronalib
//
//  Created by Randy Jones on 9/10/2018
//

#pragma once

#include <chrono>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "MLPlatform.h"
#include "MLSharedResource.h"

using namespace std::chrono;

namespace ml
{
// A simple, low-resolution timer for doing applicaton and UI tasks.
// Any callbacks are called synchronously from a single thread, so
// callbacks should not take too much time. To trigger an action
// that might take longer, send a message from the callback and
// then receive it and do the action in a private thread.

class Timer;

class Timers
{
  friend class Timer;

 public:
  static const int kMillisecondsResolution;

  Timers() {}
  ~Timers()
  {
    if (_running) stop();
  }

  // To start it running, call start() on the single Timers
  // instance. If runInMainThread is true, the timers will
  // be called from the application's main thread, on operating
  // systems that have this concept.
  void start(bool runInMainThread = false);
  void stop();

  void insert(Timer* t) { timerPtrs.insert(t); }

  void erase(Timer* t) { timerPtrs.erase(t); }

  void tick(void);
  void run(void);

 private:
  std::mutex mSetMutex;

  void* pTimersRef{nullptr};
  bool _running{false};
  bool _inMainThread{false};
  std::set<Timer*> timerPtrs;
  std::thread runThread;

#if ML_WINDOWS
  int _mainTimerID{0};
#endif
};  // class Timers

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
  Timer& operator=(Timer&&) = delete;       // Move assign

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

  // start calling the function periodically. the wait period happens before the
  // first call.
  void start(std::function<void(void)> f, const milliseconds period)
  {
    mCounter = -1;
    myFunc = f;
    mPeriod = period;
    mPreviousCall = system_clock::now();
  }

  bool isActive() { return mCounter != 0; }

  void stop();

 private:
  std::mutex _counterMutex;

  ml::SharedResourcePointer<ml::Timers> _timers;
  int mCounter{0};
  std::function<void(void)> myFunc;
  milliseconds mPeriod;
  time_point<system_clock> mPreviousCall;
};
}  // namespace ml
