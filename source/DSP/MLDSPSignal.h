// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// just a starting point! An audio Signal class. Some of the old Matrix code might be useful here.

#pragma once

#include <algorithm>
#include <vector>

namespace ml
{

struct Signal
{
  size_t channels{0};
  size_t sampleRate{0};
  std::vector< float > data;
};

inline float findMaximumSample(const Signal& x)
{
  return *std::max_element(x.data.begin(), x.data.end());
}

inline void normalize(Signal& x)
{
  float ratio = 1.0f / findMaximumSample(x);
  for(int i=0; i<x.data.size(); ++i)
  {
    x.data[i] *= ratio;
  }
}

inline void clear(Signal& x)
{
  x.data.clear();
}


}  // namespace ml
