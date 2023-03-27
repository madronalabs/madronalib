// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "madronalib.h"
#include "mldsp.h"

namespace ml
{

// TODO: refactor to store normalized and plain values. We get values (from Widgets)
// a lot more often than we set them! so optimize for getting.
// this simplifies the code: just one kind of tree.


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
      b.normalizedToPlain = projections::zero;
      b.plainToNormalized = projections::zero;
    }
    else
    {
      size_t stepCount = nItems - 1;
      b.normalizedToPlain = [=](float x) { return (floorf(fmin(stepCount, x * nItems))); };
      b.plainToNormalized = [=](float x) { return (x / stepCount); };
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
using ParameterDescriptionList = std::vector<std::unique_ptr<ParameterDescription> >;

// An annotated Tree of parameters.
class ParameterTree
{
public:
  Tree<std::unique_ptr<ParameterDescription> > descriptions;
  Tree<ParameterProjection> projections;
  
  
  void set(Path pname, Value val)
  {
    params_[pname] = val;
    
    // TODO one class
    //paramsNormalized_[pname] = val;
    //paramsPlain_[pname] = val;
  }

  virtual void setFromNormalizedValue(Path pname, Value val) = 0;
  virtual void setFromPlainValue(Path pname, Value val) = 0;
  
  Value getValue(Path pname)
  {
    return params_[pname];
  }
  
  virtual float getNormalizedFloatValue(Path pname) const = 0;
  virtual float getPlainFloatValue(Path pname) const = 0;
  virtual Value getNormalizedValue(Path pname) const = 0;
  virtual Value getPlainValue(Path pname) const = 0;

  inline void setFromPlainValues(const Tree<Value>& t)
  {
    for (auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      setFromPlainValue(valName, *it);
    }
  }
  
  inline void setFromNormalizedValues(const Tree<Value>& t)
  {
    for (auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      setFromNormalizedValue(valName, *it);
    }
  }
  
  virtual Tree<Value> getNormalizedValues() const = 0;
  virtual Tree<Value> getPlainValues() const = 0;

  void setWatchParameter(Path pname) {watchParameter = pname;}
  
protected:
  Path watchParameter{};
  
  
  Tree<Value> params_;
  
};

inline float convertNormalizedToPlainFloatValue(const ParameterTree& params, Path pname, Value val)
{
  float newNormValue = val.getFloatValue();
  float newPlainValue{0};
  auto& pdesc = params.descriptions[pname];
  bool useListValues = pdesc->getProperty("use_list_values_as_int").getBoolValue();
  if(useListValues)
  {
    auto listItems = textUtils::split(pdesc->getTextProperty("listitems"), '/');
    size_t itemIndex = params.projections[pname].normalizedToPlain(newNormValue);
    newPlainValue = textUtils::textToNaturalNumber(listItems[itemIndex]);
  }
  else
  {
    newPlainValue = params.projections[pname].normalizedToPlain(newNormValue);
  }
  return newPlainValue;
}

inline Value convertNormalizedToPlainValue(const ParameterTree& params, Path pname, Value val)
{
  if (val.isFloatType())
  {
    return Value(convertNormalizedToPlainFloatValue(params, pname, val));
  }
  else
  {
    return val;
  }
}

inline float convertPlainToNormalizedFloatValue(const ParameterTree& params, Path pname, Value val)
{
  float newPlainValue = val.getFloatValue();
  
  float newNormalizedValue{0};
  auto& pdesc = params.descriptions[pname];
  bool useListValues = pdesc->getProperty("use_list_values_as_int").getBoolValue();
  if(useListValues)
  {
    auto listItems = textUtils::split(pdesc->getTextProperty("listitems"), '/');
    
    // get item matching plain value
    for(int i=0; i<listItems.size(); ++i)
    {
      int intItem = textUtils::textToNaturalNumber(listItems[i]);
      if(newPlainValue == intItem)
      {
        newNormalizedValue = params.projections[pname].plainToNormalized(i);
        break;
      }
    }
  }
  else
  {
    newNormalizedValue = params.projections[pname].plainToNormalized(newPlainValue);
  }
  return newNormalizedValue;
}

inline Value convertPlainToNormalizedValue(const ParameterTree& params, Path pname, Value val)
{
  if (val.isFloatType())
  {
    return Value(convertPlainToNormalizedFloatValue(params, pname, val));
  }
  else
  {
    return val;
  }
}

// An annotated Tree of parameters stored as normalized values.
struct ParameterTreeNormalized : public ParameterTree
{
  inline void setFromNormalizedValue(Path pname, Value val) override
  {
    params_[pname] = val;
    
#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << "[norml paramTree set " << pname << " -> " << val << "]\n";
    }
#endif
  }
  
  inline void setFromPlainValue(Path pname, Value val) override
  {
    if (val.isFloatType())
    {
      params_[pname] = projections[pname].plainToNormalized(val.getFloatValue());
    }
    else
    {
      params_[pname] = (val);
    }
#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << "[norml paramTree set " << pname << " -> " << val << "]\n";
    }
#endif
  }
  
  inline Value getNormalizedValue(Path pname) const override
  {
    return params_[pname];
  }

  inline float getNormalizedFloatValue(Path pname) const override
  {
    return params_[pname].getFloatValue();
  }
  
  // also see above: refactor to store both normalized + plain!
  //
  inline Value getPlainValue(Path pname) const override
  {
    return convertNormalizedToPlainValue(*this, pname, params_[pname]);
  }
  
  inline float getPlainFloatValue(Path pname) const override
  {
    return convertNormalizedToPlainFloatValue(*this, pname, params_[pname]);
  }
  
  Tree<Value> getNormalizedValues() const override
  {
    return params_;
  }
  
  Tree<Value> getPlainValues() const override
  {
    Tree<Value> ret;
    const Tree<Value>& t = params_;
    for (auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      ret[valName] = getPlainValue(valName);
    }
    return ret;
  }
};

// An annotated Tree of parameters stored as plain values.
struct ParameterTreePlain : public ParameterTree
{
  inline void setFromNormalizedValue(Path pname, Value val) override
  {
    auto newVal = convertNormalizedToPlainValue(*this, pname, val);
    params_[pname] = newVal;
    
#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << "[plain paramTree set " << pname << " -> " << newVal << "]\n";
    }
#endif
  }
  
  inline void setFromPlainValue(Path pname, Value val) override
  {
    params_[pname] = val;
    
#ifdef DEBUG
    if(pname == watchParameter)
    {
      std::cout << "[plain paramTree set " << pname << " -> " << val << "]\n";
    }
#endif
  }
  
  inline Value getNormalizedValue(Path pname) const override
  {
    return convertPlainToNormalizedValue(*this, pname, params_[pname]);
  }
  
  inline float getNormalizedFloatValue(Path pname) const override
  {
    return convertPlainToNormalizedFloatValue(*this, pname, params_[pname]);
  }
  

  inline Value getPlainValue(Path pname) const override
  {
    return params_[pname];
  }
  
  inline float getPlainFloatValue(Path pname) const override
  {
    return params_[pname].getFloatValue();
  }

  Tree<Value> getNormalizedValues() const override
  {
    Tree<Value> ret;
    const Tree<Value>& t = params_;
    for (auto it = t.begin(); it != t.end(); ++it)
    {
      Path valName = it.getCurrentNodePath();
      ret[valName] = getNormalizedValue(valName);
    }
    return ret;
  }
  
  Tree<Value> getPlainValues() const override
  {
    return params_;
  }
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
    return p.projections[pname].plainToNormalized(defaultVal.getFloatValue());
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

}  // namespace ml
