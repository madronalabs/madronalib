// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLTimer.h"

#include <chrono>
#include <functional>

#include "MLPlatform.h"
using namespace std::chrono;

// Timers

const int ml::Timers::kMillisecondsResolution = 16;

#if ML_MAC

#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>

void macTimersCallback(CFRunLoopTimerRef, void* info)
{
  ml::Timers* pTimers = static_cast<ml::Timers*>(info);
  pTimers->tick();
}

void ml::Timers::start(bool runInMainThread)
{
  _inMainThread = runInMainThread;
  if (!_running)
  {
    if (_inMainThread)
    {
      CFRunLoopTimerContext timerContext = {};
      timerContext.info = this;
      float intervalInSeconds = ml::Timers::kMillisecondsResolution * 0.001f;
      pTimersRef =
          CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + intervalInSeconds,
                               intervalInSeconds, 0, 0, macTimersCallback, &timerContext);
      if (pTimersRef)
      {
        _running = true;
        CFRunLoopTimerRef pTimerRef = static_cast<CFRunLoopTimerRef>(pTimersRef);
        CFRunLoopAddTimer(CFRunLoopGetMain(), pTimerRef, kCFRunLoopCommonModes);
      }
    }
    else
    {
      _running = true;
      runThread = std::thread{[&]() { run(); }};
    }
  }
}

void ml::Timers::stop()
{
  if (_inMainThread)
  {
    if (pTimersRef)
    {
      CFRunLoopTimerRef pLoopRef = static_cast<CFRunLoopTimerRef>(pTimersRef);
      CFRunLoopRemoveTimer(CFRunLoopGetMain(), pLoopRef, kCFRunLoopCommonModes);
      pTimersRef = nullptr;
    }
  }
  else
  {
    // signal thread to exit
    _running = false;
    runThread.join();
  }
}

void ml::Timers::run(void)
{
  while (_running)
  {
    std::this_thread::sleep_for(milliseconds(Timers::kMillisecondsResolution));
    tick();
  }
}

#elif ML_WINDOWS

#include <windows.h>

ml::Timers* pWinTimers{nullptr};

void CALLBACK winTimersCallback(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR idEvent, DWORD /*dwTime*/)
{
  // ml::Timers* pTimers = (ml::Timers*)(ml::Timers::winTimersRef);
  if (pWinTimers)
  {
    pWinTimers->tick();
  }
}

void ml::Timers::start(bool runInMainThread)
{
  _inMainThread = runInMainThread;
  if (!_running)
  {
    if (_inMainThread)
    {
      _mainTimerID = SetTimer(0, 1, ml::Timers::kMillisecondsResolution, winTimersCallback);
      if (_mainTimerID)
      {
        _running = true;
        pWinTimers = this;
      }
    }
    else
    {
      _running = true;
      runThread = std::thread{[&]() { run(); }};
      pWinTimers = this;
    }
  }
}

void ml::Timers::stop()
{
  if (_running)
  {
    if (_inMainThread)
    {
      KillTimer(0, _mainTimerID);
      _running = false;
    }
    else
    {
      // signal thread to exit
      _running = false;

      // wait for exit
      runThread.join();
    }
  }
}

void ml::Timers::run(void)
{
  while (_running)
  {
    std::this_thread::sleep_for(milliseconds(Timers::kMillisecondsResolution));
    tick();
  }
}

#elif ML_LINUX

void ml::Timers::start(bool runInMainThread)
{
  if (!_running)
  {
    _running = true;
    runThread = std::thread{[&]() { run(); }};
  }
}

void ml::Timers::run(void)
{
  while (_running)
  {
    std::this_thread::sleep_for(milliseconds(Timers::kMillisecondsResolution));
    tick();
  }
}

void ml::Timers::stop(void)
{
  if (_running)
  {
    _running = false;
    runThread.join();
  }
}

#endif

void ml::Timers::tick(void)
{
  static time_point<system_clock> previousStatsTime = system_clock::now();
  time_point<system_clock> now = system_clock::now();

  std::unique_lock<std::mutex> lock(mSetMutex);
  for (auto t : _timerPtrs)
  {
    std::unique_lock<std::mutex> lock(t->_counterMutex);
    if (t->_counter != 0)
    {
      if (t->_additionalTime > milliseconds(0))
      {
        t->_previousCall = now + t->_additionalTime - t->_period;
        t->_additionalTime = milliseconds(0);
      }
      else if (now - t->_previousCall >= t->_period)
      {
        t->_func();
        if (t->_counter > 0)
        {
          t->_counter--;
        }
        t->_previousCall = now;
      }
    }
  }
}

// Timer

ml::Timer::Timer() noexcept
{
  std::unique_lock<std::mutex> lock(_timers->mSetMutex);
  _previousCall = _creationTime = system_clock::now();
  _timers->insert(this);
}

ml::Timer::~Timer()
{
  std::unique_lock<std::mutex> lock(_timers->mSetMutex);
  _timers->erase(this);
}

void ml::Timer::stop()
{
  // More lightweight ways of handling a race on _counter are possible, but as
  // stopping a timer is an infrequent operation we use the mutex for brevity.
  std::unique_lock<std::mutex> lock(_counterMutex);
  _counter = 0;
}
