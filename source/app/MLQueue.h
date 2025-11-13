// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
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

  // return the exponent of the smallest power of 2 that is >= x.
  inline size_t bitsToContain(int x)
  {
    int exp;
    for (exp = 0; (1 << exp) < x; exp++);
    return (exp);
  }

  void resize(size_t capacity)
  {
    // when readIndex_ = writeIndex_ the queue is considered empty. So
    // a queue of size 1 can't store any elements: we have to add
    // 1 to the requested capacity. Then, find the power of two size that
    // will contain that many elements.
    size_t powerOfTwoSize = 1ULL << bitsToContain((int)capacity + 1);

    data_.resize(powerOfTwoSize);
    sizeMask_ = powerOfTwoSize - 1;
    clear();
  }

  size_t size() { return data_.size(); }

  bool push(const Element& item)
  {
    const auto currentWriteIndex = writeIndex_.load(std::memory_order_relaxed);
    const auto nextWriteIndex = increment(currentWriteIndex);
    if (nextWriteIndex != readIndex_.load(std::memory_order_acquire))
    {
      data_[currentWriteIndex] = item;
      writeIndex_.store(nextWriteIndex, std::memory_order_release);
      return true;
    }
    return false;
  }

  bool pop(Element& item)
  {
    const auto currentReadIndex = readIndex_.load(std::memory_order_relaxed);
    if (currentReadIndex == writeIndex_.load(std::memory_order_acquire))
    {
      return false;  // empty queue
    }
    item = data_[currentReadIndex];
    readIndex_.store(increment(currentReadIndex), std::memory_order_release);
    return true;
  }

  Element pop()
  {
    const auto currentReadIndex = readIndex_.load(std::memory_order_relaxed);
    if (currentReadIndex == writeIndex_.load(std::memory_order_acquire))
    {
      return Element();  // empty queue, return null object
    }
    Element r = data_[currentReadIndex];
    readIndex_.store(increment(currentReadIndex), std::memory_order_release);
    return r;
  }

  void clear()
  {
    Element dummy;
    while (elementsAvailable()) pop(dummy);
  }

  size_t elementsAvailable() const
  {
    return (writeIndex_.load(std::memory_order_acquire) -
            readIndex_.load(std::memory_order_relaxed)) &
           sizeMask_;
  }

  // useful for reading elements while a criterion is met. Can be used like
  // while queue.elementsAvailable() && q.peek().mTime < 100 { q.pop(elem) ... }
  const Element& peek() const
  {
    const auto currentReadIndex = readIndex_.load(std::memory_order_relaxed);
    return data_[currentReadIndex];
  }

  bool wasEmpty() const { return (writeIndex_.load() == readIndex_.load()); }

  bool wasFull() const
  {
    const auto nextWriteIndex = increment(writeIndex_.load());
    return (nextWriteIndex == readIndex_.load());
  }

 private:
  size_t increment(size_t idx) const { return (idx + 1) & sizeMask_; }

  std::vector<Element> data_;
  size_t sizeMask_;
  std::atomic<size_t> writeIndex_{0};
  std::atomic<size_t> readIndex_{0};
};
};  // namespace ml
