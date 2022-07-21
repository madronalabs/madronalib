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
  Tree< Value > properties;

 public:
  PropertyTree() = default;
  PropertyTree(Tree<Value> vt) : properties(vt) {}
  PropertyTree(const PropertyTree& other) : properties(other.properties) {}
  PropertyTree(const std::initializer_list<NamedValue> p)
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

  float getFloatProperty(Path p) const { return properties[p].getFloatValue(); }
  bool getBoolProperty(Path p) const { return properties[p].getBoolValue(); }
  int getIntProperty(Path p) const { return properties[p].getIntValue(); }
  Text getTextProperty(Path p) const { return properties[p].getTextValue(); }
  Matrix getMatrixProperty(Path p) const { return properties[p].getMatrixValue(); }
  uint32_t getUnsignedLongProperty(Path p) const { return properties[p].getUnsignedLongValue(); }

  float getFloatPropertyWithDefault(Path p, float d) const
  {
    return properties[p].getFloatValueWithDefault(d);
  }
  bool getBoolPropertyWithDefault(Path p, bool d) const
  {
    return properties[p].getBoolValueWithDefault(d);
  }
  int getIntPropertyWithDefault(Path p, int d) const
  {
    return properties[p].getIntValueWithDefault(d);
  }
  Text getTextPropertyWithDefault(Path p, Text d) const
  {
    return properties[p].getTextValueWithDefault(d);
  }
  Matrix getMatrixPropertyWithDefault(Path p, Matrix d) const
  {
    return properties[p].getMatrixValueWithDefault(d);
  }
  uint32_t getUnsignedLongPropertyWithDefault(Path p, uint32_t d) const
  {
    return properties[p].getUnsignedLongValueWithDefault(d);
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
      setProperty(it.getCurrentNodePath(), *it);
    }
  }

  void dump() { properties.dump(); }

  inline Tree<Value>::const_iterator begin() const { return properties.begin(); }

  inline Tree<Value>::const_iterator end() const { return properties.end(); }
};

}  // namespace ml
