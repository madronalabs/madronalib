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
  
  // return the exponent of the smallest power of 2 that is >= x.
  inline size_t bitsToContain(int x)
  {
    int exp;
    for (exp = 0; (1 << exp) < x; exp++)
    ;
    return (exp);
  }

  void resize(size_t capacity)
  {
    // when _readIndex = _writeIndex the queue is considered empty. So
    // a queue of size 1 can't store any elements: we have to add
    // 1 to the requested capacity. Then, find the power of two size that
    // will contain that many elements.
    size_t powerOfTwoSize = 1 << bitsToContain(capacity + 1);
    
    _data.resize(powerOfTwoSize);
    _sizeMask = powerOfTwoSize - 1;
    clear();
  }
  
  size_t size()
  {
    return _data.size();
  }
  
  bool push(const Element& item)
  {
    const auto currentWriteIndex = _writeIndex.load(std::memory_order_relaxed);
    const auto nextWriteIndex = increment(currentWriteIndex);
    if (nextWriteIndex != _readIndex.load(std::memory_order_acquire))
    {
      _data[currentWriteIndex] = item;
      _writeIndex.store(nextWriteIndex, std::memory_order_release);
      return true;
    }
    return false;
  }

  bool pop(Element& item)
  {
    const auto currentReadIndex = _readIndex.load(std::memory_order_relaxed);
    if (currentReadIndex == _writeIndex.load(std::memory_order_acquire))
    {
      return false;  // empty queue
    }
    item = _data[currentReadIndex];
    _readIndex.store(increment(currentReadIndex), std::memory_order_release);
    return true;
  }

  Element pop()
  {
    const auto currentReadIndex = _readIndex.load(std::memory_order_relaxed);
    if (currentReadIndex == _writeIndex.load(std::memory_order_acquire))
    {
      return Element();  // empty queue, return null object
    }
    Element r = _data[currentReadIndex];
    _readIndex.store(increment(currentReadIndex), std::memory_order_release);
    return r;
  }

  void clear()
  {
    Element dummy;
    while (elementsAvailable()) pop(dummy);
  }

  size_t elementsAvailable() const
  {
    return (_writeIndex.load(std::memory_order_acquire) - _readIndex.load(std::memory_order_relaxed)) & _sizeMask;
  }

  // useful for reading elements while a criterion is met. Can be used like
  // while queue.elementsAvailable() && q.peek().mTime < 100 { q.pop(elem) ... }
  const Element& peek() const
  {
    const auto currentReadIndex = _readIndex.load(std::memory_order_relaxed);
    return _data[currentReadIndex];
  }

  bool wasEmpty() const { return (_writeIndex.load() == _readIndex.load()); }

  bool wasFull() const
  {
    const auto nextWriteIndex = increment(_writeIndex.load());
    return (nextWriteIndex == _readIndex.load());
  }

 private:
  size_t increment(size_t idx) const { return (idx + 1) & _sizeMask; }

  std::vector<Element> _data;
  size_t _sizeMask;
  std::atomic<size_t> _writeIndex{0};
  std::atomic<size_t> _readIndex{0};
};
};  // namespace ml
