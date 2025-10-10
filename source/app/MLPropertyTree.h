// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLSerialization.h"
#include "MLTree.h"
#include "MLValue.h"

namespace ml
{
class PropertyTree
{
  Tree< Value > properties;
  
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
  
  // basics
  
  bool hasProperty(Path p) const { return(properties.getNode(p) != nullptr); }
  void setProperty(Path p, Value v) { properties[p] = v; }
  Value getProperty(Path p) const { return properties[p]; }
  
  // getters for basic parameter value types
  
  float getFloatProperty(Path p) const { return properties[p].getFloatValue(); }
  float getFloatPropertyWithDefault(Path p, float d) const
  {
    auto treeNode = properties.getNode(p);
    return treeNode ? treeNode->getValue().getFloatValue() : d;
  }
  
  double getDoubleProperty(Path p) const { return properties[p].getDoubleValue(); }
  double getDoublePropertyWithDefault(Path p, double d) const
  {
    auto treeNode = properties.getNode(p);
    return treeNode ? treeNode->getValue().getDoubleValue() : d;
  }
  
  bool getBoolProperty(Path p) const { return properties[p].getBoolValue(); }
  bool getBoolPropertyWithDefault(Path p, bool d) const
  {
    auto treeNode = properties.getNode(p);
    return treeNode ? treeNode->getValue().getBoolValue() : d;
  }
  
  int getIntProperty(Path p) const { return properties[p].getIntValue(); }
  int32_t getIntPropertyWithDefault(Path p, int32_t d) const
  {
    auto treeNode = properties.getNode(p);
    return treeNode ? treeNode->getValue().getIntValue() : d;
  }
  
  Text getTextProperty(Path p) const { return properties[p].getTextValue(); }
  Text getTextPropertyWithDefault(Path p, Text d) const
  {
    auto treeNode = properties.getNode(p);
    return treeNode ? treeNode->getValue().getTextValue() : d;
  }
  
  // utility getters for other types
  
  Interval getIntervalProperty(Path p) const { return valueToPODType<Interval>(getProperty(p)); }
  Interval getIntervalPropertyWithDefault(Path p, Interval d) const { return hasProperty(p) ? getIntervalProperty(p) : d; }
  inline void setIntervalProperty(Path p, Interval v) { setProperty(p, podTypeToValue<Interval>(v)); }
  

  template< size_t N >
  std::array< float, N > getFloatArrayProperty(Path p) const
  {
    return properties[p].getFloatArray< N >();
  }
  
  template< size_t N >
  std::array< float, N > getFloatArrayPropertyWithDefault(Path p, std::array< float, N > d) const
  {
    return hasProperty(p) ? getFloatArrayProperty< N >(p) : d;
  }
  
  
  // serialization
  
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
  
  // iterators
  
  inline Tree<Value>::const_iterator begin() const { return properties.begin(); }
  inline Tree<Value>::const_iterator end() const { return properties.end(); }
};

}  // namespace ml
