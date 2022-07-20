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
    // get number of items
    size_t nItems{0};
    if(p.hasProperty("listitems"))
    {
      // read and count list items
      auto listItems = textUtils::split(p.getTextProperty("listitems"), '/');
      nItems = listItems.size();
    }
    else if(p.hasProperty("num_items"))
    {
      nItems = p.getFloatProperty("num_items");
    }
    
    if(nItems <= 1)
    {
      b.normalizedToPlain = projections::zero;
      b.plainToNormalized = projections::zero;
    }
    else
    {
      size_t stepCount = nItems - 1;
      b.normalizedToPlain = [=](float x) { return(floorf(fmin(stepCount, x*nItems))); };
      b.plainToNormalized = [=](float x) { return(x/stepCount); };
    }
  }
  else
  {
    if (bLog)
    {
      b.normalizedToPlain =
      ml::projections::intervalMap(normalRange, plainRange, ml::projections::log(plainRange));
      b.plainToNormalized =
      ml::projections::intervalMap(plainRange, normalRange, ml::projections::exp(plainRange));
      
     // std::cout << "plainToNormalized: " << 
      
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
  
  virtual void setParamFromNormalizedValue(Path pname, Value val) = 0;
  virtual void setParamFromPlainValue(Path pname, Value val) = 0;

  
  virtual float getNormalizedValue(Path pname) const = 0;
  virtual float getPlainValue(Path pname) const = 0;

  inline void setFromPlainValues(const Tree< Value >& t)
  {
    for(auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      setParamFromPlainValue(valName, *it);
    }
  }
  
  inline void setFromNormalizedValues(const Tree< Value >& t)
  {
    for(auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      setParamFromNormalizedValue(valName, *it);
    }
  }
  
  virtual Tree< Value > getNormalizedValues() const = 0;
  virtual Tree< Value > getPlainValues()  const = 0;
};

// An annotated Tree of parameters stored as normalized values.
struct ParameterTreeNormalized : public ParameterTree
{
  inline void setParamFromNormalizedValue(Path pname, Value val) override
  {
    (*this)[pname] = val;
  }
  
  inline void setParamFromPlainValue(Path pname, Value val) override
  {
    if(val.isFloatType())
    {
      (*this)[pname] = projections[pname].plainToNormalized(val.getFloatValue());
    }
    else
    {
      (*this)[pname] = (val);
    }
  }

  inline float getNormalizedValue(Path pname) const override
  {
    return (*this)[pname].getFloatValue();
  }
  
  // TODO this returns float, not Value, which makes other code messy. refactor
  inline float getPlainValue(Path pname) const override
  {
    return projections[pname].normalizedToPlain((*this)[pname].getFloatValue());
  }
  
  Tree< Value > getNormalizedValues() const override
  {
    return (*this);
  }
  
  Tree< Value > getPlainValues() const override
  {
    Tree< Value > ret;
    const Tree< Value >& t = (*this);
    for(auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      Value val = *it;
      if(val.isFloatType())
      {
        ret[valName] = getPlainValue(valName);
      }
      else
      {
        ret[valName] = val;
      }
    }
    return ret;
  }
};

// An annotated Tree of parameters stored as plain values.
struct ParameterTreePlain : public ParameterTree
{
  inline void setParamFromNormalizedValue(Path pname, Value val) override
  {
    if(val.isFloatType())
    {
      (*this)[pname] = projections[pname].normalizedToPlain(val.getFloatValue());
    }
    else
    {
      (*this)[pname] = (val);
    }
  }
  
  inline void setParamFromPlainValue(Path pname, Value val) override
  {
    (*this)[pname] = val;
  }
  
  inline float getNormalizedValue(Path pname) const override
  {
    return projections[pname].plainToNormalized((*this)[pname].getFloatValue());
  }
  
  inline float getPlainValue(Path pname) const override
  {
    return (*this)[pname].getFloatValue();
  }
  
  Tree< Value > getNormalizedValues() const override
  {
    Tree< Value > ret;
    const Tree< Value >& t = (*this);
    for(auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      Value val = *it;
      if(val.isFloatType())
      {
        ret[valName] = getNormalizedValue(valName);
      }
      else
      {
        ret[valName] = val;
      }
    }
    return ret;
  }
  
  Tree< Value > getPlainValues() const override
  {
    return (*this);
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
      p.setParamFromNormalizedValue(pname, paramDesc->getProperty("default"));
    }
    else if(paramDesc->hasProperty("plaindefault"))
    {
      p.setParamFromPlainValue(pname, paramDesc->getProperty("plaindefault"));
    }
    else
    {
      p.setParamFromNormalizedValue(pname, 0.5f);
    }
  }
}

inline float getPlainValue(const ParameterTree& p, Path pname)
{
  return p.getPlainValue(pname);
}

inline float getNormalizedValue(const ParameterTree& p, Path pname)
{
  return p.getNormalizedValue(pname);
}

inline Tree< Value > getPlainValues(const ParameterTree& p)
{
  return p.getPlainValues();
}

inline Tree< Value > getNormalizedValues(const ParameterTree& p)
{
  return p.getNormalizedValues();
}

}  // namespace ml
