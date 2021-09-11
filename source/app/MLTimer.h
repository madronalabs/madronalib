// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <chrono>
#include <functional>
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

  void insert(Timer* t) { _timerPtrs.insert(t); }
  void erase(Timer* t) { _timerPtrs.erase(t); }

  void tick(void);
  void run(void);
  
  // MLTEST
  size_t getSize() { return _timerPtrs.size(); }

 private:
  std::mutex mSetMutex;

  void* pTimersRef{nullptr};
  bool _running{false};
  bool _inMainThread{false};
  std::set<Timer*> _timerPtrs;
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
    _counter = 1;
    _func = f;
    _period = period;
    _previousCall = system_clock::now();
  }
  
  // extend the timeout of the current period for the given number of
  // milliseconds after the next tick.
  void postpone(const milliseconds timeToAdd)
  {
    _additionalTime = timeToAdd;
  }
  
  // call the function n times, waiting the specified interval before each.
  void callNTimes(std::function<void(void)> f, const milliseconds period, int n)
  {
    _counter = n;
    _func = f;
    _period = period;
    _previousCall = system_clock::now();
  }

  // start calling the function periodically. the wait period happens before the
  // first call.
  void start(std::function<void(void)> f, const milliseconds period)
  {
    _counter = -1;
    _func = f;
    _period = period;
    _previousCall = system_clock::now();
  }

  bool isActive() { return _counter != 0; }

  void stop();
  
  int _testID{}; // MLTEST

 private:
  std::mutex _counterMutex;

  ml::SharedResourcePointer<ml::Timers> _timers;
  int _counter{0};
  std::function<void(void)> _func;
  milliseconds _period{};
  milliseconds _additionalTime{};
  time_point<system_clock> _previousCall{};
  time_point<system_clock> _creationTime{};
};
}  // namespace ml
