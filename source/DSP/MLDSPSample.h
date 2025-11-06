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
  std::vector<float> sampleData;

  float operator[](size_t i) const { return sampleData[i]; }
  float& operator[](size_t i) { return sampleData[i]; }
};

inline size_t getSize(const Sample& s) { return s.sampleData.size(); }

inline size_t getFrames(const Sample& s)
{
  if (s.channels == 0) return 0;
  return s.sampleData.size() / s.channels;
}

inline const float* getConstFramePtr(const Sample& s, size_t frameIdx = 0)
{
  return s.sampleData.data() + frameIdx * s.channels;
}

inline float* getFramePtr(Sample& s, size_t frameIdx = 0)
{
  return s.sampleData.data() + frameIdx * s.channels;
}

inline float getRate(const Sample& s) { return s.sampleRate; }

inline float getDuration(const Sample& s)
{
  if (s.sampleRate == 0) return 0.f;
  return getFrames(s) / (float)(s.sampleRate);
}

inline bool usable(const Sample* pSample)
{
  if (!pSample) return false;
  return pSample->sampleData.size() > 0;
}

inline float* resize(Sample& s, size_t newFrames, size_t newChans = 1)
{
  float* r{nullptr};
  try
  {
    s.sampleData.resize(newFrames * newChans);
  }
  catch (...)
  {
    return nullptr;
  }
  s.channels = newChans;
  return s.sampleData.data();
}

inline float findMaximumValue(const Sample& x)
{
  return *std::max_element(x.sampleData.begin(), x.sampleData.end());
}

inline void normalize(Sample& x)
{
  if (x.sampleData.size() == 0) return;
  float ratio = 1.0f / findMaximumValue(x);
  for (int i = 0; i < x.sampleData.size(); ++i)
  {
    x.sampleData[i] *= ratio;
  }
}

inline void clear(Sample& x) { x.sampleData.clear(); }

}  // namespace ml
