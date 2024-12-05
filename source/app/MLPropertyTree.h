// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLSerialization.h"
#include "MLTree.h"
#include "MLValue.h"

namespace ml
{
class PropertyTree
{
  Tree<Value> properties;
  
  // Properties can provide converters to other types commonly used in Widgets

 public:
  PropertyTree() = default;
  PropertyTree(Tree<Value> vt) : properties(vt) {}
  PropertyTree(const PropertyTree& other) : properties(other.properties) {}
  
  PropertyTree(const std::initializer_list< NamedValue >& p)
  {
    for (const auto& v : p)
    {
      properties.add(v.name, v.value);
    }
  }
  
  bool hasProperty(Path p) const { return (properties.getConstNode(p) != nullptr); }

  // get the Value of the property. Will return a null Value object if no such
  // property exists.
  Value getProperty(Path p) const { return properties[p]; }
  void setProperty(Path p, Value v) { properties[p] = v; }
  
  // getters for basic parameter value types

  float getFloatProperty(Path p) const { return properties[p].getFloatValue(); }
  bool getBoolProperty(Path p) const { return properties[p].getBoolValue(); }
  int getIntProperty(Path p) const { return properties[p].getInt32Value(); }
  Text getTextProperty(Path p) const { return properties[p].getTextValue(); }
  uint32_t getUnsignedLongProperty(Path p) const { return properties[p].getUInt32Value(); }

  float getFloatPropertyWithDefault(Path p, float d) const
  {
    auto treeNode = properties.getConstNode(p);
    return treeNode ? treeNode->getValue().getFloatValue() : d;
  }
  
  bool getBoolPropertyWithDefault(Path p, bool d) const
  {
    auto treeNode = properties.getConstNode(p);
    return treeNode ? treeNode->getValue().getBoolValue() : d;
  }

  int32_t getIntPropertyWithDefault(Path p, int32_t d) const
  {
    auto treeNode = properties.getConstNode(p);
    return treeNode ? treeNode->getValue().getInt32Value() : d;
  }
  
  Text getTextPropertyWithDefault(Path p, Text d) const
  {
    auto treeNode = properties.getConstNode(p);
    return treeNode ? treeNode->getValue().getTextValue() : d;
  }
  
  uint32_t getUnsignedLongPropertyWithDefault(Path p, uint32_t d) const
  {
    auto treeNode = properties.getConstNode(p);
    return treeNode ? treeNode->getValue().getUInt32Value() : d;
  }

  
  Interval getIntervalProperty(Path p) const
  {
    auto floatPtr = properties[p].getFloatArrayPtr();
    return Interval{floatPtr[0], floatPtr[1]};
  }
  Interval getIntervalPropertyWithDefault(Path p, Interval d) const
  {
    Interval r;
    auto treeNode = properties.getConstNode(p);
    if(treeNode)
    {
      auto floatPtr = treeNode->getValue().getFloatArrayPtr();
      r = Interval{floatPtr[0], floatPtr[1]};
    }
    else
    {
      r = d;
    }
    return r;
  }



  std::vector<unsigned char> propertyTreeToBinary() { return valueTreeToBinary(properties); }
  PropertyTree binaryToPropertyTree(const std::vector<unsigned char>& binaryData)
  {
    return PropertyTree(binaryToValueTree(binaryData));
  }

  void overwrite(const PropertyTree& other)
  {
    for (auto it = other.properties.begin(); it != other.properties.end(); ++it)
    {
      setProperty(it.getCurrentPath(), *it);
    }
  }

  void dump() { properties.dump(); }

  inline Tree<Value>::const_iterator begin() const { return properties.begin(); }

  inline Tree<Value>::const_iterator end() const { return properties.end(); }
  
  // TODO why does PropertyTree not provide a [] operator?
};

}  // namespace ml
