// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <array>

#include "MLDSPBuffer.h"
#include "MLDSPProjections.h"

namespace ml
{
inline void mapIndices(float* pDest, size_t size, Projection p)
{
  for (int i = 0; i < size; ++i)
  {
    pDest[i] = p((float)i);
  }
}

inline void makeWindow(float* pDest, size_t size, Projection windowShape)
{
  auto domainToUnity = projections::linear({0.f, size - 1.f}, {0.f, 1.f});
  mapIndices(pDest, size, compose(windowShape, domainToUnity));
}

namespace dspwindows
{
const Projection rectangle([](float x) { return (x > 0.75f) ? 0.f : ((x < 0.25f) ? 0.f : 1.f); });
const Projection triangle([](float x) { return (x > 0.5f) ? (2.f - 2.f * x) : (2.f * x); });
const Projection raisedCosine([](float x) { return 0.5f - 0.5f * cosf(kTwoPi * x); });
const Projection hamming([](float x) { return 0.54f - 0.46f * cosf(kTwoPi * x); });
const Projection blackman(
    [](float x) { return 0.42f - 0.5f * cosf(kTwoPi * x) + 0.08f * cosf(2.f * kTwoPi * x); });
const Projection flatTop(
    [](float x)
    {
      const float a0 = 0.21557895f;
      const float a1 = 0.41663158f;
      const float a2 = 0.277263158f;
      const float a3 = 0.083578947f;
      const float a4 = 0.006947368f;
      return a0 - a1 * cosf(kTwoPi * x) + a2 * cosf(2.f * kTwoPi * x) -
             a3 * cosf(3.f * kTwoPi * x) + a4 * cosf(4.f * kTwoPi * x);
    });
}  // namespace dspwindows

// UsingFlushDenormalsToZero: turn off denormal math so that (for example) IIR filters don't consume
// many more CPU cycles when they decay. Only needed for Intel processors.
struct UsingFlushDenormalsToZero
{
#if defined(__SSE__)
  uint32_t MXCRState;

  UsingFlushDenormalsToZero()
  {
    // Set the DAZ (denormals are zero) and FZ (flush to zero) in the Intel MXCSR register
    MXCRState = _mm_getcsr();           // read the old MXCSR setting
    int newMXCSR = MXCRState | 0x8040;  // set DAZ and FZ bits
    _mm_setcsr(newMXCSR);               // write the new MXCSR setting to the MXCSR
  }

  ~UsingFlushDenormalsToZero()
  {
    // restore the old MXCSR setting
    _mm_setcsr(MXCRState);
  }

#elif defined(__aarch64__)
  uint64_t MXCRState = 0;

  UsingFlushDenormalsToZero()
  {
    // read and store floating point control register (FPCR)
    uint64_t FPCR_prev = 0;
    asm volatile("MRS %0, FPCR " : "=r"(FPCR_prev));
    MXCRState = FPCR_prev;

    // set flush to zero bit and write FPCR
    uint64_t FPCR = FPCR_prev | (1ULL << 24);
    asm volatile("MSR FPCR, %0 " : : "r"(FPCR));
  }

  ~UsingFlushDenormalsToZero()
  {
    // restore the old MXCSR setting
    asm volatile("MSR FPCR, %0 " : : "r"(MXCRState));
  }

#else
  UsingFlushDenormalsToZero() = default;
  ~UsingFlushDenormalsToZero() = default;

#endif
};

}  // namespace ml
