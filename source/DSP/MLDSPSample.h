// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// just a starting point! An audio sample class. Some of the old Matrix code might be useful here.

#pragma once

#include <algorithm>
#include <vector>

namespace ml
{

struct Sample
{
  size_t channels{0};
  size_t sampleRate{0};
  std::vector<float> data;
};

inline float findMaximumValue(const Sample& x)
{
  return *std::max_element(x.data.begin(), x.data.end());
}

inline void normalize(Sample& x)
{
  float ratio = 1.0f / findMaximumValue(x);
  for (int i = 0; i < x.data.size(); ++i)
  {
    x.data[i] *= ratio;
  }
}

inline void clear(Sample& x) { x.data.clear(); }

}  // namespace ml
