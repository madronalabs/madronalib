#pragma once

#include <DSP/MLDSPOps.h>

#include <array>
#include <cassert>
#include <functional>

namespace ml
{

// A generic resampler that allows you to process data at one sample
// rate and automatically convert the result to another sample rate by
// stretching or squashing the frames. Frames are linearly interpolated.
//
// Example usage:
// 
// static const float SOURCE_SAMPLE_RATE = 44100.0f;
// 
// auto source = []() -> ml::DSPVector
// {
//   // Generate sine data at 44100hz
//   return sineGen(440.0f / SOURCE_SAMPLE_RATE);
// };
// 
// // Convert to the current sample rate. The perceived sine data will be the same pitch
// auto resampledOutput = resampler(source, SOURCE_SAMPLE_RATE / currentSampleRate);

template <size_t ROWS>
class DSPResampler
{
 public:
  using Source = std::function<ml::DSPVectorArray<ROWS>()>;

  ml::DSPVectorArray<ROWS> operator()(Source source, float factor)
  {
    ml::DSPVectorArray<ROWS> out;

    for (int i = 0; i < kFloatsPerDSPVector; i++)
    {
      const auto frame = readSourceBlockFrame(source, mFramePos);

      mFramePos += factor;

      for (int r = 0; r < ROWS; r++)
      {
        out.row(r)[i] = frame[r];
      }
    }

    return out;
  }

 private:
  std::array<float, ROWS> readSourceBlockFrame(Source source, double pos)
  {
    const auto idxPrev = static_cast<std::uint32_t>(std::floor(pos));
    const auto idxNext = static_cast<std::uint32_t>(std::ceil(pos));
    const auto x = static_cast<float>(pos - idxPrev);

    const auto prev = readSourceBlockFrame(source, idxPrev);
    const auto next = readSourceBlockFrame(source, idxNext);

    std::array<float, ROWS> out;

    for (int r = 0; r < ROWS; r++)
    {
      out[r] = ml::lerp(prev[r], next[r], x);
    }

    return out;
  }

  std::array<float, ROWS> readSourceBlockFrame(Source source, std::uint32_t index)
  {
    const auto blockIndex = index / kFloatsPerDSPVector;
    const auto localIndex = index % kFloatsPerDSPVector;

    assert(blockIndex >= mCurrBlockIndex - 1);
    assert(blockIndex <= mCurrBlockIndex + 1);

    if (blockIndex < mCurrBlockIndex)
    {
      return readSourceBlockFrame(mPrevSourceBlock, localIndex);
    }

    if (blockIndex > mCurrBlockIndex)
    {
      readNextSourceBlock(source);
    }

    return readSourceBlockFrame(mCurrSourceBlock, localIndex);
  }

  std::array<float, ROWS> readSourceBlockFrame(const ml::DSPVectorArray<ROWS>& block, int index)
  {
    std::array<float, ROWS> out;

    for (int r = 0; r < ROWS; r++)
    {
      out[r] = block.constRow(r)[index];
    }

    return out;
  }

  void readNextSourceBlock(Source source)
  {
    mPrevSourceBlock = mCurrSourceBlock;
    mCurrSourceBlock = source();
    mCurrBlockIndex++;
  }

  double mFramePos = 0.0;
  ml::DSPVectorArray<ROWS> mPrevSourceBlock;
  ml::DSPVectorArray<ROWS> mCurrSourceBlock;
  std::int64_t mCurrBlockIndex = -1;
};

}  // namespace ml