//
//  MLLocks.h
//  madronalib
//
//  Created by Randy Jones on 9/21/15.
//
//

#ifndef __MLLocks__
#define __MLLocks__

#include <atomic>
#include <memory>

class Lockable
{    
public:
	Lockable() {}
	virtual ~Lockable() {}
	
	virtual void enter() const = 0;
	//bool tryEnter() = 0;
	virtual void exit() const = 0;
};

class MLSpinLock : public Lockable
{
public:
	MLSpinLock() 
	{
		// Windows-compatible way of initializing lock
		mBusyFlag.clear(std::memory_order_release); 
	}
	
	virtual ~MLSpinLock() {}

	void enter() const
	{
		while(mBusyFlag.test_and_set(std::memory_order_acquire));
	}
	
	void exit() const
	{
		mBusyFlag.clear(std::memory_order_release);
	}

	std::atomic_flag mBusyFlag;
};

class MLScopedLock
{
public:
	inline MLScopedLock(const Lockable& lock)
	: mLock(lock)
	{
		mLock.enter(); 		
	}
	
	inline ~MLScopedLock()
	{
		mLock.exit();
	}
	
	const Lockable& mLock;
};

#endif /* defined(__MLLocks__) */
