/*
	Copyright (c) 2009 Remy Muller. 
	
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files
	(the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge,
	publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so,
	subject to the following conditions:
	
	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.
	
	Any person wishing to distribute modifications to the Software is
	requested to send the modifications to the original developer so that
	they can be incorporated into the canonical version.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
	ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "zeroconf/Thread.h"
#include <cassert>

using namespace ZeroConf;

#ifdef WIN32

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <process.h>

static unsigned int __stdcall threadEntryPoint (void* userData)
{
  Thread::threadEntryPoint((Thread*)userData);

  _endthreadex (0);
  return 0;
}

static void *createThread(void *pUser)
{
  unsigned int threadId;

  return (void*) _beginthreadex (0, 0, &threadEntryPoint, pUser, 0, &threadId);
}

static void killThread(void *pHandle)
{
  if (pHandle != 0)
  {
    TerminateThread (pHandle, 0);
  }
}

static void threadSleep(int ms)
{
  Sleep(ms);
}

static void closeThread(void *pHandle)
{
  CloseHandle ((HANDLE) pHandle);
}

#else

static void *threadEntryPoint(void *pUser)
{
  //const ScopedAutoReleasePool pool;
  Thread::threadEntryPoint((Thread*)pUser);
  return NULL;
}

static void *createThread(void *pUser)
{
  pthread_t handle = 0;

  if (pthread_create (&handle, 0, threadEntryPoint, pUser) == 0)
  {
    pthread_detach (handle);
    return (void*) handle;
  }

  return 0;
}

static void killThread(void *pHandle)
{
  if (pHandle != NULL)
    pthread_cancel ((pthread_t) pHandle);
}

static void threadSleep(int ms)
{
  struct timespec time;
  time.tv_sec = ms / 1000;
  time.tv_nsec = (ms % 1000) * 1000000;
  nanosleep (&time, 0);
}

static void closeThread(void *pHandle)
{
  // nothing to do on posix
}

#endif

Thread::Thread()
: mpThreadHandle(NULL)
, mShouldExit(false)
{
}

Thread::~Thread()
{
  stopThread(100);
}

void Thread::startThread()
{
  const ScopedLock lock(mCriticalSection);

  mShouldExit = false;
  if(mpThreadHandle == NULL)
  {
    mpThreadHandle = createThread(this);
  }
}

void Thread::stopThread(const int timeOut)
{
  const ScopedLock lock(mCriticalSection);

  if(isThreadRunning())
  {
    setThreadShouldExit();
    // notify

    if(timeOut != 0)
      waitForThreadToExit(timeOut);

    if(isThreadRunning())
    {
      killThread(mpThreadHandle);
      mpThreadHandle = NULL;
    }
  }
}


bool Thread::isThreadRunning() const
{
  return mpThreadHandle != NULL;
}

bool Thread::threadShouldExit() const
{
  return mShouldExit;
}

void Thread::setThreadShouldExit()
{
  mShouldExit = true;
}

bool Thread::waitForThreadToExit(const int timeOut)
{
  int count = timeOut;

  while (isThreadRunning()) {
    if(timeOut>0 && --count < 0)
      return false;

    sleep(1);
  }

  return true;
}

void Thread::sleep(int ms)
{
  threadSleep(ms);
}


void Thread::threadEntryPoint(Thread *pThread)
{
  pThread->run();

  closeThread(pThread->mpThreadHandle);

  pThread->mpThreadHandle = NULL;
}
