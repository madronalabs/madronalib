// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

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
    if (running_) stop();
  }

  // To start it running, call start() on the single Timers
  // instance. If runInMainThread is true, the timers will
  // be called from the application's main thread, on operating
  // systems that have this concept.
  void start(bool runInMainThread = false);
  void stop();

  void insert(Timer* t) { timerPtrs_.insert(t); }
  void erase(Timer* t) { timerPtrs_.erase(t); }

  void tick(void);
  void run(void);

  // MLTEST
  size_t getSize() { return timerPtrs_.size(); }

 private:
  std::mutex setMutex_;

  void* pTimersRef{nullptr};
  bool running_{false};
  bool inMainThread_{false};
  std::set<Timer*> timerPtrs_;
  std::thread runThread;

#if ML_WINDOWS
  int mainTimerID_{0};
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
    counter_ = 1;
    func_ = f;
    period_ = period;
    previousCall_ = system_clock::now();
  }

  // extend the timeout of the current period for the given number of
  // milliseconds after the next tick.
  void postpone(const milliseconds timeToAdd) { additionalTime_ = timeToAdd; }

  // call the function n times, waiting the specified interval before each.
  void callNTimes(std::function<void(void)> f, const milliseconds period, int n)
  {
    counter_ = n;
    func_ = f;
    period_ = period;
    previousCall_ = system_clock::now();
  }

  // start calling the function periodically. the wait period happens before the
  // first call.
  void start(std::function<void(void)> f, const milliseconds period)
  {
    counter_ = -1;
    func_ = f;
    period_ = period;
    previousCall_ = system_clock::now();
  }

  bool isActive() { return counter_ != 0; }

  void stop();

 private:
  std::mutex counterMutex_;

  SharedResourcePointer<Timers> timers_;
  int counter_{0};
  std::function<void(void)> func_;
  milliseconds period_{};
  milliseconds additionalTime_{};
  time_point<system_clock> previousCall_{};
  time_point<system_clock> creationTime_{};
};
}  // namespace ml
