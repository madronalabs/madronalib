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
  Projection normalizedToPlain{projections::unity};
  Projection plainToNormalized{projections::unity};
};

inline ParameterProjection createParameterProjection(const ParameterDescription& p)
{
  ParameterProjection b;
  bool bLog = p.getProperty("log").getBoolValueWithDefault(false);
  Matrix range = p.getProperty("range").getMatrixValueWithDefault({0, 1});
  Interval fullRange{range[0], range[1]};

  if (bLog)
  {
    b.normalizedToPlain =
        ml::projections::intervalMap({0, 1}, fullRange, ml::projections::log(fullRange));
    b.plainToNormalized =
        ml::projections::intervalMap(fullRange, {0, 1}, ml::projections::exp(fullRange));
  }
  else
  {
    b.normalizedToPlain = projections::linear({0, 1}, fullRange);
    b.plainToNormalized = projections::linear(fullRange, {0, 1});
  }
  return b;
}


// A list of Parameter descriptions.
using ParameterDescriptionList = std::vector< std::unique_ptr< ParameterDescription > >;

// An annotated Tree of parameters.
struct ParameterTree : public Tree< Value >
{
  Tree< std::unique_ptr< ParameterDescription > > descriptions;
  Tree< ParameterProjection > projections;
  
  virtual void setParamFromNormalizedValue(Path pname, float val) = 0;
  virtual void setParamFromPlainValue(Path pname, float val) = 0;
  virtual float getNormalizedValue(Path pname) = 0;
  virtual float getPlainValue(Path pname) = 0;
};

// An annotated Tree of parameters stored as normalized values.
struct ParameterTreeNormalized : public ParameterTree
{
  inline void setParamFromNormalizedValue(Path pname, float val) override
  {
    (*this)[pname] = val;
  }
  inline void setParamFromPlainValue(Path pname, float val) override
  {
    (*this)[pname] = projections[pname].plainToNormalized(val);
  }
  inline float getNormalizedValue(Path pname) override
  {
    return (*this)[pname].getFloatValue();
  }
  inline float getPlainValue(Path pname) override
  {
    return projections[pname].normalizedToPlain((*this)[pname].getFloatValue());
  }
};

// An annotated Tree of parameters stored as plain values.
struct ParameterTreePlain : public ParameterTree
{
  inline void setParamFromNormalizedValue(Path pname, float val) override
  {
    (*this)[pname] = projections[pname].normalizedToPlain(val);
  }
  inline void setParamFromPlainValue(Path pname, float val) override
  {
    (*this)[pname] = val;
  }
  inline float getNormalizedValue(Path pname) override
  {
    return projections[pname].plainToNormalized((*this)[pname].getFloatValue());
  }
  inline float getPlainValue(Path pname) override
  {
    return (*this)[pname].getFloatValue();
  }
};


// functions on ParameterTrees.

inline void addParameterToTree(const ParameterDescription& paramDesc, ParameterTree& paramTree)
{
  auto paramName = paramDesc.getTextProperty("name");
  paramTree.projections[paramName] = createParameterProjection(paramDesc);
  paramTree.descriptions[paramName] = ml::make_unique< ParameterDescription >(paramDesc);
}

inline void buildParameterTree(const ParameterDescriptionList& paramList, ParameterTree& paramTree)
{
  for(const auto& paramDesc : paramList)
  {
    addParameterToTree(*paramDesc, paramTree);
  }
}

inline void setDefaults(ParameterTree& p)
{
  for(auto& paramDesc : p.descriptions)
  {
    Path pname = paramDesc->getTextProperty("name");
    if(pname)
    {
      float defaultVal = paramDesc->getFloatPropertyWithDefault("default", 0.5f);
      p.setParamFromNormalizedValue(pname, defaultVal);
    }
  }
}

inline float getPlainValue(ParameterTree& p, Path pname)
{
  return p.getPlainValue(pname);
}

inline float getNormalizedValue(ParameterTree& p, Path pname)
{
  return p.getNormalizedValue(pname);
}

}  // namespace ml
