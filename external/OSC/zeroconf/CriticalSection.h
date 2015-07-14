#ifndef ZeroConf_CriticalSection_H
#define ZeroConf_CriticalSection_H

#ifndef WIN32
#include <pthread.h>
#endif

namespace ZeroConf 
{
  class CriticalSection
  {
  public:
    CriticalSection();
    ~CriticalSection();

    void enter() const;
    bool tryEnter() const;
    void exit() const;

  private:
#ifdef WIN32
    char details[44];
#else
    mutable pthread_mutex_t details;
#endif
  };

  class ScopedLock
  {
  public:
    inline ScopedLock (const CriticalSection& lock) 
      : mLock(lock) 
    {
      mLock.enter(); 
    }
    inline ~ScopedLock() 
    {
      mLock.exit(); 
    }
  private:
    const CriticalSection& mLock;
  };
}

#endif
