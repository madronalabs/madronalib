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
  auto units = Symbol(p.getProperty("units").getTextValue());
  bool bLog = p.getProperty("log").getBoolValueWithDefault(false);
  Matrix range = p.getProperty("range").getMatrixValueWithDefault({0, 1});
  
  Interval normalRange{0., 1.};
  Interval plainRange{range[0], range[1]};

  // make ranges for list parameters
  if(units == "list")
  {
    if(p.hasProperty("listitems"))
    {
      // read and count list items
      // list params don't need a range property
      auto listItems = textUtils::split(p.getTextProperty("listitems"), '/');
      plainRange = Interval{-0.5f, listItems.size() - 0.5f};
      
      // normal range: e.g. 0.25 -- 0.75 for 2 items, to center norm value in item
      // float halfItem = 0.5f/(listItems.size());
      normalRange = Interval{0.f, 1.f};
    }
    else
    {
      //TODO error handling
      std::cout << "parameter description " << p.getProperty("name") << ": no list items!\n";
    }
    b.normalizedToPlain = projections::linear(normalRange, plainRange);
    b.plainToNormalized = projections::linear(plainRange, normalRange);
  }
  else
  {
    if (bLog)
    {
      b.normalizedToPlain =
          ml::projections::intervalMap(normalRange, plainRange, ml::projections::log(plainRange));
      b.plainToNormalized =
          ml::projections::intervalMap(plainRange, normalRange, ml::projections::exp(plainRange));
    }
    else
    {
      b.normalizedToPlain = projections::linear(normalRange, plainRange);
      b.plainToNormalized = projections::linear(plainRange, normalRange);
    }
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

// set the description of the parameter paramName in the tree paramTree to paramDesc.
inline void setParameterInfo(ParameterTree& paramTree, Path paramName, const ParameterDescription& paramDesc)
{
  paramTree.projections[paramName] = createParameterProjection(paramDesc);
  paramTree.descriptions[paramName] = ml::make_unique< ParameterDescription >(paramDesc);
}

inline void buildParameterTree(const ParameterDescriptionList& paramList, ParameterTree& paramTree)
{
  for(const auto& paramDesc : paramList)
  {
    setParameterInfo(paramTree, paramDesc->getTextProperty("name"), *paramDesc);
  }
}

inline void setDefaults(ParameterTree& p)
{
  for(auto& paramDesc : p.descriptions)
  {
    Path pname = paramDesc->getTextProperty("name");
    
    if(paramDesc->hasProperty("default"))
    {
      p.setParamFromNormalizedValue(pname, paramDesc->getFloatProperty("default"));
    }
    else if(paramDesc->hasProperty("plaindefault"))
    {
      p.setParamFromPlainValue(pname, paramDesc->getFloatProperty("plaindefault"));
    }
    else
    {
      p.setParamFromNormalizedValue(pname, 0.5f);
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
