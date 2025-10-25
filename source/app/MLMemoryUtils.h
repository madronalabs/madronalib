// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <algorithm>
#include <assert.h>

namespace ml
{

// SmallStackBuffer - allocate some memory on the stack if we don't need much,
// otherwise use the heap.

template <class T, int MAX_STACK_ELEMS>
class SmallStackBuffer
{
 public:
  SmallStackBuffer(size_t size)
  {
    if (size <= MAX_STACK_ELEMS)
    {
      mpData = mLocalData;
      std::fill(mpData, mpData + size, T());
    }
    else
    {
      mpData = new T[size];
    }
  }

  ~SmallStackBuffer()
  {
    if (mpData != mLocalData)
    {
      delete[] mpData;
    }
  }

  T* data() { return mpData; }

 private:
  T* mpData;
  T mLocalData[MAX_STACK_ELEMS];
};


inline int sizeToInt(size_t size)
{
  assert(size <= static_cast<size_t>(std::numeric_limits<int>::max()) &&
         "size_t value too large for int");
  return static_cast<int>(size);
}

}  // namespace ml
