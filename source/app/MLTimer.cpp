// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
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
  inMainThread_ = runInMainThread;
  if (!running_)
  {
    if (inMainThread_)
    {
      CFRunLoopTimerContext timerContext = {};
      timerContext.info = this;
      float intervalInSeconds = ml::Timers::kMillisecondsResolution * 0.001f;
      pTimersRef =
          CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + intervalInSeconds,
                               intervalInSeconds, 0, 0, macTimersCallback, &timerContext);
      if (pTimersRef)
      {
        running_ = true;
        CFRunLoopTimerRef pTimerRef = static_cast<CFRunLoopTimerRef>(pTimersRef);
        CFRunLoopAddTimer(CFRunLoopGetMain(), pTimerRef, kCFRunLoopCommonModes);
      }
    }
    else
    {
      running_ = true;
      runThread = std::thread{[&]() { run(); }};
    }
  }
}

void ml::Timers::stop()
{
  if (inMainThread_)
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
    running_ = false;
    runThread.join();
  }
}

void ml::Timers::run(void)
{
  while (running_)
  {
    std::this_thread::sleep_for(milliseconds(Timers::kMillisecondsResolution));
    tick();
  }
}

#elif ML_WINDOWS

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
  inMainThread_ = runInMainThread;
  if (!running_)
  {
    if (inMainThread_)
    {
      mainTimerID_ = SetTimer(0, 1, ml::Timers::kMillisecondsResolution, winTimersCallback);
      if (mainTimerID_)
      {
        running_ = true;
        pWinTimers = this;
      }
    }
    else
    {
      running_ = true;
      runThread = std::thread{[&]() { run(); }};
      pWinTimers = this;
    }
  }
}

void ml::Timers::stop()
{
  if (running_)
  {
    if (inMainThread_)
    {
      KillTimer(0, mainTimerID_);
      running_ = false;
    }
    else
    {
      // signal thread to exit
      running_ = false;

      // wait for exit
      runThread.join();
    }
  }
}

void ml::Timers::run(void)
{
  while (running_)
  {
    std::this_thread::sleep_for(milliseconds(Timers::kMillisecondsResolution));
    tick();
  }
}

#elif ML_LINUX

void ml::Timers::start(bool runInMainThread)
{
  if (!running_)
  {
    running_ = true;
    runThread = std::thread{[&]() { run(); }};
  }
}

void ml::Timers::run(void)
{
  while (running_)
  {
    std::this_thread::sleep_for(milliseconds(Timers::kMillisecondsResolution));
    tick();
  }
}

void ml::Timers::stop(void)
{
  if (running_)
  {
    running_ = false;
    runThread.join();
  }
}

#endif

void ml::Timers::tick(void)
{
  static time_point<system_clock> previousStatsTime = system_clock::now();
  time_point<system_clock> now = system_clock::now();

  std::unique_lock<std::mutex> lock(setMutex_);
  for (auto t : timerPtrs_)
  {
    std::unique_lock<std::mutex> lock(t->counterMutex_);
    if (t->counter_ != 0)
    {
      if (t->additionalTime_ > milliseconds(0))
      {
        t->previousCall_ = now + t->additionalTime_ - t->period_;
        t->additionalTime_ = milliseconds(0);
      }
      else if (now - t->previousCall_ >= t->period_)
      {
        t->func_();
        if (t->counter_ > 0)
        {
          t->counter_--;
        }
        t->previousCall_ = now;
      }
    }
  }
}

// Timer

ml::Timer::Timer() noexcept
{
  std::unique_lock<std::mutex> lock(timers_->setMutex_);
  previousCall_ = creationTime_ = system_clock::now();
  timers_->insert(this);
}

ml::Timer::~Timer()
{
  std::unique_lock<std::mutex> lock(timers_->setMutex_);
  timers_->erase(this);
}

void ml::Timer::stop()
{
  // More lightweight ways of handling a race on counter_ are possible, but as
  // stopping a timer is an infrequent operation we use the mutex for brevity.
  std::unique_lock<std::mutex> lock(counterMutex_);
  counter_ = 0;
}
