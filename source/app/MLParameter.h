// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "mldsp.h"
#include "madronalib.h"

namespace ml {

using ParameterDescription = PropertyTree;

struct ParameterProjection
{
  Projection normalizedToReal{projections::unity};
  Projection realToNormalized{projections::unity};
};

inline ParameterProjection createParameterProjection(ParameterDescription p)
{
  ParameterProjection b;
  bool bLog = p.getProperty("log").getBoolValueWithDefault(false);
  Matrix range = p.getProperty("range").getMatrixValueWithDefault({0, 1});
  Interval fullRange{range[0], range[1]};
  
  if(bLog)
  {
    b.normalizedToReal = ml::projections::intervalMap({0, 1}, fullRange, ml::projections::log(fullRange));
    b.realToNormalized = ml::projections::intervalMap(fullRange, {0, 1}, ml::projections::exp(fullRange));
  }
  else
  {
    b.normalizedToReal = projections::linear({0, 1}, fullRange);
    b.realToNormalized = projections::linear(fullRange, {0, 1});
  }
  return b;
}

} // namespaces

