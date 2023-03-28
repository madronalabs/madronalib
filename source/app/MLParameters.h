// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "madronalib.h"
#include "mldsp.h"

namespace ml
{

using ParameterDescription = PropertyTree;

using ParameterDescriptionList = std::vector< std::unique_ptr< ParameterDescription> >;

struct ParameterProjection
{
  Projection normalizedToReal{projections::unity};
  Projection realToNormalized{projections::unity};
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
  if (units == "list")
  {
    // get number of items
    size_t nItems{0};
    if (p.hasProperty("listitems"))
    {
      // read and count list items
      auto listItems = textUtils::split(p.getTextProperty("listitems"), '/');
      nItems = listItems.size();
    }
    else if (p.hasProperty("num_items"))
    {
      nItems = p.getFloatProperty("num_items");
    }
    
    if (nItems <= 1)
    {
      b.normalizedToReal = projections::zero;
      b.realToNormalized = projections::zero;
    }
    else
    {
      size_t stepCount = nItems - 1;
      b.normalizedToReal = [=](float x) { return (floorf(fmin(stepCount, x * nItems))); };
      b.realToNormalized = [=](float x) { return (x / stepCount); };
    }
  }
  else
  {
    if(bLog)
    {
      b.normalizedToReal =
      ml::projections::intervalMap(normalRange, plainRange, ml::projections::log(plainRange));
      b.realToNormalized =
      ml::projections::intervalMap(plainRange, normalRange, ml::projections::exp(plainRange));
    }
    else
    {
      b.normalizedToReal = projections::linear(normalRange, plainRange);
      b.realToNormalized = projections::linear(plainRange, normalRange);
    }
  }
  return b;
}

// An annotated Tree of parameters.
class ParameterTree
{
public:
  Tree< std::unique_ptr< ParameterDescription > > descriptions;
  Tree< ParameterProjection > projections;
  Tree< Value > paramsNorm_;
  Tree< Value > paramsReal_;
  
  float convertNormalizedToRealFloatValue(Path pname, Value val) const
  {
    float newNormValue = val.getFloatValue();
    float newRealValue{0};
    auto& pdesc = descriptions[pname];
    bool useListValues = pdesc->getProperty("use_list_values_as_int").getBoolValue();
    if(useListValues)
    {
      auto listItems = textUtils::split(pdesc->getTextProperty("listitems"), '/');
      size_t itemIndex = projections[pname].normalizedToReal(newNormValue);
      newRealValue = textUtils::textToNaturalNumber(listItems[itemIndex]);
    }
    else
    {
      newRealValue = projections[pname].normalizedToReal(newNormValue);
    }
    return newRealValue;
  }

  float convertRealToNormalizedFloatValue(Path pname, Value val) const
  {
    float newRealValue = val.getFloatValue();
    
    float newNormalizedValue{0};
    auto& pdesc = descriptions[pname];
    bool useListValues = pdesc->getProperty("use_list_values_as_int").getBoolValue();
    if(useListValues)
    {
      auto listItems = textUtils::split(pdesc->getTextProperty("listitems"), '/');
      
      // get item matching plain value
      for(int i=0; i<listItems.size(); ++i)
      {
        int intItem = textUtils::textToNaturalNumber(listItems[i]);
        if(newRealValue == intItem)
        {
          newNormalizedValue = projections[pname].realToNormalized(i);
          break;
        }
      }
    }
    else
    {
      newNormalizedValue = projections[pname].realToNormalized(newRealValue);
    }
    return newNormalizedValue;
  }
  
  inline Value convertNormalizedToRealValue(Path pname, Value val) const
  {
    if (val.isFloatType())
    {
      return Value(convertNormalizedToRealFloatValue(pname, val));
    }
    else
    {
      return val;
    }
  }
  
  inline Value convertRealToNormalizedValue(Path pname, Value val) const
  {
    if (val.isFloatType())
    {
      return Value(convertRealToNormalizedFloatValue(pname, val));
    }
    else
    {
      return val;
    }
  }
  
  // set a parameter's real value without conversion. For params that don't have normal values.
  // both normal and real params are set for ease of getting all normalized + non-normalizable values.
  void setRealValue(Path pname, Value val)
  {
    paramsNorm_[pname] = val;
    paramsReal_[pname] = val;
  }
  
  Value::Type getValueType(Path pname) const
  {
    return paramsReal_[pname].getType();
  }
  
  Value getRealValue(Path pname) const
  {
    return paramsReal_[pname];
  }
  
  Value getNormalizedValue(Path pname) const
  {
    return paramsNorm_[pname];
  }
  
  float getRealFloatValue(Path pname) const
  {
    return paramsReal_[pname].getFloatValue();
  }
  
  float getNormalizedFloatValue(Path pname) const
  {
    return paramsNorm_[pname].getFloatValue();
  }

  inline void setFromNormalizedValue(Path pname, Value val)
  {
    paramsNorm_[pname] = val;
    paramsReal_[pname] = convertNormalizedToRealValue(pname, val);

#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << "[paramTree set from norm " << pname << " -> " << val << "]\n";
    }
#endif
  }
  
  inline void setFromRealValue(Path pname, Value val)
  {
    paramsReal_[pname] = val;
    paramsNorm_[pname] = convertRealToNormalizedValue(pname, val);
    
#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << "[paramTree set from real " << pname << " -> " << val << "]\n";
    }
#endif
  }
  
  inline void setFromNormalizedValues(const Tree<Value>& t)
  {
    for (auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      setFromNormalizedValue(valName, *it);
    }
  }

  inline void setFromRealValues(const Tree<Value>& t)
  {
    for (auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      setFromRealValue(valName, *it);
    }
  }
  
  const Tree<Value>& getNormalizedValues() const
  {
    return paramsNorm_;
  }
  
  const Tree<Value>& getRealValues() const
  {
    return paramsReal_;
  }

  void setWatchParameter(Path pname) {watchParameter = pname;}
  
protected:
  Path watchParameter{};
};



// functions on ParameterTrees.

// set the description of the parameter paramName in the tree paramTree to paramDesc.
inline void setParameterInfo(ParameterTree& paramTree, Path paramName,
                             const ParameterDescription& paramDesc)
{
  paramTree.projections[paramName] = createParameterProjection(paramDesc);
  paramTree.descriptions[paramName] = ml::make_unique<ParameterDescription>(paramDesc);
}

// get default parameter value in normalized units.
inline Value getNormalizedDefaultValue(ParameterTree& p, Path pname)
{
  const auto& paramDesc = p.descriptions[pname];
  
  if (paramDesc->hasProperty("default"))
  {
    return paramDesc->getProperty("default");
  }
  else if (paramDesc->hasProperty("plaindefault"))
  {
    // convert plain default to normalized and return
    Value defaultVal = paramDesc->getProperty("plaindefault");
    return p.projections[pname].realToNormalized(defaultVal.getFloatValue());
  }
  else if (paramDesc->hasProperty("range"))
  {
    // if the param has a range, we assume it's a float param and return 0.5.
    return Value(0.5f);
  }
  else
  {
    // since there's no param value yet, we really don't know anything about
    // the default.
    return Value();
  }
}

inline void setDefault(ParameterTree& p, Path pname)
{
  Value v = getNormalizedDefaultValue(p, pname);
  p.setFromNormalizedValue(pname, v);
}

inline void buildParameterTree(const ParameterDescriptionList& paramList, ParameterTree& paramTree)
{
  for (const auto& paramDesc : paramList)
  {
    auto pname = paramDesc->getTextProperty("name");
    setParameterInfo(paramTree, pname, *paramDesc);
  }
}

inline void setDefaults(ParameterTree& p)
{
  for (auto& paramDesc : p.descriptions)
  {
    Path pname = paramDesc->getTextProperty("name");
    setDefault(p, pname);
  }
}

/*
inline Value getPlainValue(const ParameterTree& p, Path pname)
{
  return p.getPlainValue(pname);
}

inline Value getNormalizedValue(const ParameterTree& p, Path pname)
{
  return p.getNormalizedValue(pname);
}

inline Tree<Value> getPlainValues(const ParameterTree& p)
{
  return p.getPlainValues();
}

inline Tree<Value> getNormalizedValues(const ParameterTree& p)
{
  return p.getNormalizedValues();
}
*/

}  // namespace ml
