// madronaLib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <functional>
#include <vector>

#include "MLDSPScalarMath.h"

namespace ml
{
struct Interval
{
  float mX1, mX2;
};

inline bool within(float f, const Interval m) { return (f >= m.mX1) && (f < m.mX2); }

using Projection = std::function<float(float)>;

inline Projection compose(Projection a, Projection b)
{
  return [=](float x) { return a(b(x)); };
}

namespace projections
{
// useful projections with no parameters defined on (0, 1)

static const Projection unity{[](float x) { return x; }};
static const Projection squared{[](float x) { return x * x; }};
static const Projection flip{[](float x) { return 1 - x; }};
static const Projection clip{[](float x) { return ml::clamp(x, 0.f, 1.f); }};
static const Projection smoothstep{[](float x) { return 3 * x * x - 2 * x * x * x; }};
static const Projection flatcenter{[](float x) {
  float c = (x - 0.5f);
  return 4 * c * c * c + 0.5f;
}};
static const Projection bell{[](float x) {
  float px = x * 2 - 1;
  return powf(2.f, -(10.f * px * px));
}};
static const Projection easeOut{[](float x) {
  float m = x - 1;
  return 1 - m * m;
}};
static const Projection easeIn{[](float x) { return x * x; }};
static const Projection easeInOut{[](float x) {
  return (x < 0.5f) ? easeIn(x * 2.f) * 0.5f : easeOut(x * 2.f - 1.f) * 0.5f + 0.5f;
}};
static const Projection easeOutCubic{[](float x) {
  float n = 1 - x;
  return 1 - n * n * n;
}};
static const Projection easeInCubic{[](float x) { return x * x * x; }};
static const Projection easeInOutCubic{[](float x) {
  return (x < 0.5f) ? easeInCubic(x * 2.f) * 0.5f : easeOutCubic(x * 2.f - 1.f) * 0.5f + 0.5f;
}};
static const Projection easeOutQuartic{[](float x) {
  float m = x - 1;
  return 1 - m * m * m * m;
}};
static const Projection easeInQuartic{[](float x) { return x * x * x * x; }};
static const Projection easeInOutQuartic{[](float x) {
  return (x < 0.5f) ? easeInQuartic(x * 2.f) * 0.5f : easeOutQuartic(x * 2.f - 1.f) * 0.5f + 0.5f;
}};

// functions taking one or more parameters and returning projections

// ml::projections::log returns a projection from [0, 1] to a logarithmic curve
// on [a, b]. works for positive a, b with a < b only.
inline Projection log(Interval m)
{
  return [=](float x) {
    float a = m.mX1;
    float b = m.mX2;
    return a * (powf((b / a), x) - 1) / (b - a);
  };
}

// the inverse of the log projection.
// works for positive a, b with a < b only.
inline Projection exp(Interval m)
{
  return [=](float x) {
    float a = m.mX1;
    float b = m.mX2;
    return logf((x * (b - a) + a) / a) / logf(b / a);
  };
}

// linear projection mapping an interval to another interval
inline Projection linear(const Interval a, const Interval b)
{
  return [=](float x) {
    // project interval a to interval b
    float m = (b.mX2 - b.mX1) / (a.mX2 - a.mX1);
    return m * (x - a.mX1) + b.mX1;
  };
}

// a projection mapping an interval to another interval with an intermediate
// shaping projection on [0, 1]
inline Projection intervalMap(const Interval a, const Interval b, Projection c)
{
  return [=](float x) {
    // project interval a to interval (0,1)
    const float scaleA = 1 / (a.mX2 - a.mX1);
    const float offsetA = (-a.mX1) / (a.mX2 - a.mX1);
    // project interval (0, 1) to interval b
    const float scaleB = (b.mX2 - b.mX1);
    const float offsetB = b.mX1;
    return c(x * scaleA + offsetA) * scaleB + offsetB;
  };
}

inline Projection piecewiseLinear(std::initializer_list<float> values)
{
  const std::vector<float> table(values);

  if (table.size() > 1)
  {
    return [=](float x) {
      float ni = table.size() - 1;
      float nf = static_cast<float>(ni);
      float xf = nf * clamp(x, 0.f, 1.f);
      int xi = static_cast<int>(xf);
      float xr = xf - xi;

      if (x < 1.0f)
      {
        return lerp(table[xi], table[xi + 1], xr);
      }
      else
      {
        return table[ni];
      }
    };
  }
  else if (table.size() == 1)
  {
    return [=](float x) { return table[0]; };
  }
  else
  {
    return [=](float x) { return 0.f; };
  }
}

// like piecewiseLinear, but with a shape for each segment for easing and such

inline Projection piecewise(std::initializer_list<float> valueList,
                            std::initializer_list<Projection> shapeList)
{
  // TODO asserts not working on Windows
  // assert(shapeList.size() == valueList.size() - 1);
  const std::vector<float> table(valueList);
  const std::vector<Projection> shapeTable(shapeList);

  if (table.size() > 1)
  {
    return [=](float x) {
      float ni = table.size() - 1;
      float nf = static_cast<float>(ni);
      float xf = nf * clamp(x, 0.f, 1.f);
      int xi = static_cast<int>(xf);
      float xr = xf - xi;

      if (x < 1.0f)
      {
        // map xr to shape
        float xrm = shapeTable[xi](xr);
        return lerp(table[xi], table[xi + 1], xrm);
      }
      else
      {
        return table[ni];
      }
    };
  }
  else if (table.size() == 1)
  {
    return [=](float x) { return table[0]; };
  }
  else
  {
    return [=](float x) { return 0.f; };
  }
}
}  // namespace projections
}  // namespace ml
