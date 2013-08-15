#include "zeroconf/CriticalSection.h"
#include <cassert>

using namespace ZeroConf;

#ifdef WIN32

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#define _WIN32_WINNT 0x0500

#include <windows.h>

CriticalSection::CriticalSection()
{
  assert (sizeof (CRITICAL_SECTION) <= sizeof (details));
  InitializeCriticalSection ((CRITICAL_SECTION*)details);
}

CriticalSection::~CriticalSection()
{
  DeleteCriticalSection ((CRITICAL_SECTION*) details);
}

void CriticalSection::enter() const
{
  EnterCriticalSection ((CRITICAL_SECTION*) details);
}

bool CriticalSection::tryEnter() const
{
  return TryEnterCriticalSection ((CRITICAL_SECTION*) details) != FALSE;
}

void CriticalSection::exit() const
{
  LeaveCriticalSection ((CRITICAL_SECTION*) details);
}

#else

CriticalSection::CriticalSection()
{
  pthread_mutexattr_t atts;
  pthread_mutexattr_init (&atts);
  pthread_mutexattr_settype (&atts, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init (&details, &atts);
}

CriticalSection::~CriticalSection()
{
  pthread_mutex_destroy (&details);
}

void CriticalSection::enter() const
{
  pthread_mutex_lock (&details);
}

bool CriticalSection::tryEnter() const
{
  return pthread_mutex_trylock (&details) == 0;
}

void CriticalSection::exit() const
{
  pthread_mutex_unlock (&details);
}
#endif