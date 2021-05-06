// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "madronalib.h"
#include "mldsp.h"

namespace ml
{
using ParameterDescription = PropertyTree;

struct ParameterProjection
{
  Projection normalizedToReal{projections::unity};
  Projection realToNormalized{projections::unity};
};

inline ParameterProjection createParameterProjection(const ParameterDescription& p)
{
  ParameterProjection b;
  bool bLog = p.getProperty("log").getBoolValueWithDefault(false);
  Matrix range = p.getProperty("range").getMatrixValueWithDefault({0, 1});
  Interval fullRange{range[0], range[1]};

  if (bLog)
  {
    b.normalizedToReal =
        ml::projections::intervalMap({0, 1}, fullRange, ml::projections::log(fullRange));
    b.realToNormalized =
        ml::projections::intervalMap(fullRange, {0, 1}, ml::projections::exp(fullRange));
  }
  else
  {
    b.normalizedToReal = projections::linear({0, 1}, fullRange);
    b.realToNormalized = projections::linear(fullRange, {0, 1});
  }
  return b;
}

// A list of Parameter descriptions.
using ParameterDescriptionList = std::vector< std::unique_ptr< ParameterDescription > >;

// An annotated Tree of parameter values.
struct ParameterTree : public Tree< Value >
{
  Tree< std::unique_ptr< ParameterDescription > > descriptions;
  Tree< std::unique_ptr< ParameterProjection > > projections;
};

inline void addParameterToTree(const ParameterDescription& paramDesc, ParameterTree& paramTree)
{
  auto paramName = paramDesc.getTextProperty("name");
  paramTree.projections[paramName] = ml::make_unique< ParameterProjection >(createParameterProjection(paramDesc));
  paramTree.descriptions[paramName] = ml::make_unique< ParameterDescription >(paramDesc);
}

inline void buildParameterTree(const ParameterDescriptionList& paramList, ParameterTree& paramTree)
{
  for(const auto& paramDesc : paramList)
  {
    addParameterToTree(*paramDesc, paramTree);
  }
}

inline void setDefaultsNormalized(ParameterTree& p)
{
  for(auto& paramDesc : p.descriptions)
  {
    Path paramName = paramDesc->getTextProperty("name");
    if(paramName)
    {
      float defaultVal = paramDesc->getFloatPropertyWithDefault("default", 0.5f);
      p[paramName] = (defaultVal);
    }
  }
}

inline void setDefaultsReal(ParameterTree& p)
{
  for(auto& paramDesc : p.descriptions)
  {
    Path paramName = paramDesc->getTextProperty("name");
    if(paramName)
    {
      float defaultVal = paramDesc->getFloatPropertyWithDefault("default", 0.5f);
      p[paramName] = p.projections[paramName]->normalizedToReal(defaultVal);
    }
  }
}

}  // namespace ml
