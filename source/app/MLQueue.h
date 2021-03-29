// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// A very simple SPSC Queue.
// based on
// https://kjellkod.wordpress.com/2012/11/28/c-debt-paid-in-full-wait-free-lock-free-queue/

#pragma once

#include <atomic>
#include <cstddef>
#include <iterator>
#include <vector>

namespace ml
{
template <typename Element>
class Queue final
{
 public:
  Queue(size_t size) { resize(size); }

  ~Queue() {}

  void resize(size_t size)
  {
    mData.resize(size + 1);
    clear();
  }

  bool push(const Element& item)
  {
    const auto currentWriteIndex = mWriteIndex.load(std::memory_order_relaxed);
    const auto nextWriteIndex = increment(currentWriteIndex);
    if (nextWriteIndex != mReadIndex.load(std::memory_order_acquire))
    {
      mData[currentWriteIndex] = item;
      mWriteIndex.store(nextWriteIndex, std::memory_order_release);
      return true;
    }
    return false;
  }

  bool pop(Element& item)
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    if (currentReadIndex == mWriteIndex.load(std::memory_order_acquire))
    {
      return false;  // empty queue
    }
    item = mData[currentReadIndex];
    mReadIndex.store(increment(currentReadIndex), std::memory_order_release);
    return true;
  }

  Element pop()
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    if (currentReadIndex == mWriteIndex.load(std::memory_order_acquire))
    {
      return Element();  // empty queue, return null object
    }
    Element r = mData[currentReadIndex];
    mReadIndex.store(increment(currentReadIndex), std::memory_order_release);
    return r;
  }

  void clear()
  {
    Element dummy;
    while (elementsAvailable()) pop(dummy);
  }

  size_t elementsAvailable() const
  {
    return (mWriteIndex.load(std::memory_order_acquire) -
            mReadIndex.load(std::memory_order_relaxed)) %
           mData.size();
  }

  // useful for reading elements while a criterion is met. Can be used like
  // while queue.elementsAvailable() && q.peek().mTime < 100 { q.pop(elem) ... }
  const Element& peek() const
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    return mData[currentReadIndex];
  }

  bool wasEmpty() const { return (mWriteIndex.load() == mReadIndex.load()); }

  bool wasFull() const
  {
    const auto nextWriteIndex = increment(mWriteIndex.load());
    return (nextWriteIndex == mReadIndex.load());
  }

 private:
  size_t increment(size_t idx) const { return (idx + 1) % (mData.size()); }

  std::vector<Element> mData;
  std::atomic<size_t> mWriteIndex{0};
  std::atomic<size_t> mReadIndex{0};
};
};  // namespace ml
