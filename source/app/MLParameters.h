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
  bool bisquare = p.getProperty("bisquare").getBoolValueWithDefault(false);
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
      projections::intervalMap(normalRange, plainRange, projections::log(plainRange));
      b.realToNormalized =
      projections::intervalMap(plainRange, normalRange, projections::exp(plainRange));
    }
    else if(bisquare)
    {
      b.normalizedToReal = compose(projections::bisquared, projections::linear(normalRange, plainRange));
      b.realToNormalized = compose(projections::linear(plainRange, normalRange), projections::invBisquared);
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
    if(!pdesc) return 0;
    
    bool useListValues = pdesc->getBoolPropertyWithDefault("use_list_values_as_int", false);
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
    float newNormValue{0};
    float newRealValue = val.getFloatValue();
    auto& pdesc = descriptions[pname];
    if(!pdesc) return 0;

    bool useListValues = pdesc->getBoolPropertyWithDefault("use_list_values_as_int", false);
    if(useListValues)
    {
      auto listItems = textUtils::split(pdesc->getTextProperty("listitems"), '/');
      
      // get item matching plain value
      for(int i=0; i<listItems.size(); ++i)
      {
        int intItem = textUtils::textToNaturalNumber(listItems[i]);
        if(newRealValue == intItem)
        {
          newNormValue = projections[pname].realToNormalized(i);
          break;
        }
      }
    }
    else
    {
      newNormValue = projections[pname].realToNormalized(newRealValue);
    }
    return newNormValue;
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
  
  // set a parameter's value without conversion. For params that don't have normalizable values.
  // both normal and real params are set for ease of getting all normalized + non-normalizable values.
  void setValue(Path pname, Value val)
  {
    paramsNorm_[pname] = val;
    paramsReal_[pname] = val;
  }
  
  inline void setFromNormalizedValue(Path pname, Value val)
  {
    paramsNorm_[pname] = val;
    paramsReal_[pname] = convertNormalizedToRealValue(pname, val);

#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << "[paramTree set from norm " << pname << " -> " << val << "/" << paramsReal_[pname] << "]\n";
    }
#endif
  }
  
  inline void setFromRealValue(Path pname, Value val)
  {
    
#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << ">>> setting from real value: " << pname << " = " <<  val << "\n";
    }
#endif
    
    paramsNorm_[pname] = convertRealToNormalizedValue(pname, val);
    paramsReal_[pname] = val;
    
#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << "[paramTree set from real " << pname << " -> " << paramsNorm_[pname] << " / " << val << "]\n";
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
  
  void dump()
  {
    std::cout << "\n----------------------------\n";
    for (auto it = descriptions.begin(); it != descriptions.end(); ++it)
    {
      const auto& paramDesc = *it;
      auto pname = paramDesc->getTextProperty("name");
      auto normVal = paramsNorm_[pname];
      auto realVal = paramsReal_[pname];
      std::cout << pname << ": " << normVal << " / " << realVal << "\n";
    }
    std::cout << "----------------------------\n\n";
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
    Value defaultVal = paramDesc->getProperty("default");
    if(defaultVal.getType() == Value::kTextValue)
    {
      // descriptions must have the default string "blob" in them to be
      // set up propery as a blob type.
      if(defaultVal == "blob")
      {
        // setup default blob
        uint32_t blobData{'test'};
        Value blobDefault(&blobData, 4);
        return blobDefault;
      }
      else
      {
        // set default text for other text parameters
        return defaultVal;
      }
    }
    else
    {
      return defaultVal;
    }
  }
  else if (paramDesc->hasProperty("plaindefault"))
  {
    // convert plain default to normalized and return
    Value defaultVal = paramDesc->getProperty("plaindefault");
    return p.convertRealToNormalizedFloatValue(pname, defaultVal.getFloatValue());
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

// returns pointer to parameter description in list matching name
inline ParameterDescription* findNamedParameter(const ParameterDescriptionList& paramList, Path pname)
{
  ParameterDescription* pParam{nullptr};
  for(int i = 0; i<paramList.size(); ++i)
  {
    auto& pDesc = paramList[i];
    if(pDesc->getTextProperty("name") == pname)
    {
      pParam = pDesc.get();
      break;
    }
  }
  return pParam;
}

}  // namespace ml
