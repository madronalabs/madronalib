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
 protected:
  Tree<Value> properties;

 public:
  PropertyTree() = default;
  PropertyTree(Tree<Value> vt) : properties(vt) {}
  PropertyTree(const PropertyTree& other) : properties(other.properties) {}
  PropertyTree(const WithValues& p)
  {
    for (const auto& v : p)
    {
      properties.add(v.name, v.value);
    }
  }

  // basics

  bool hasProperty(Path p) const { return (properties.getNode(p) != nullptr); }
  void setProperty(Path p, Value v) { properties[p] = v; }
  Value getProperty(Path p) const { return properties[p]; }

  // getters for basic parameter value types

  float getFloatProperty(Path p) const { return properties[p].getFloatValue(); }
  float getFloatPropertyWithDefault(Path p, float d) const
  {
    auto treeNode = properties.getNode(p);
    return treeNode ? treeNode->getValue().getFloatValue() : d;
  }

  bool getBoolProperty(Path p) const { return properties[p].getIntValue() != 0; }
  template <size_t N>
  bool getBoolProperty(const char (&pathStr)[N]) const
  {
    Path p(pathStr);
    return getBoolProperty(p);
  }

  bool getBoolPropertyWithDefault(Path p, bool d) const
  {
    auto treeNode = properties.getNode(p);
    return treeNode ? (treeNode->getValue().getIntValue() != 0) : d;
  }

  int getIntProperty(Path p) const { return properties[p].getIntValue(); }
  int getIntPropertyWithDefault(Path p, int d) const
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

  template <size_t N>
  std::array<float, N> getFloatArrayProperty(Path p) const
  {
    return properties[p].getFloatArray<N>();
  }

  template <size_t N>
  std::array<float, N> getFloatArrayPropertyWithDefault(Path p, std::array<float, N> d) const
  {
    return hasProperty(p) ? getFloatArrayProperty<N>(p) : d;
  }

  std::vector<float> getFloatVectorProperty(Path p) const { return properties[p].getFloatVector(); }

  // getters for other types.

  Interval getIntervalProperty(Path p) const
  {
    auto a = properties[p].getFloatArray<2>();
    return Interval{a[0], a[1]};
  }
  Interval getIntervalPropertyWithDefault(Path p, Interval d) const
  {
    auto treeNode = properties.getNode(p);
    return treeNode ? getIntervalProperty(p) : d;
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
