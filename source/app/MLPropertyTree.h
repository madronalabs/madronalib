
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLSerialization.h"
#include "MLTree.h"
#include "MLValue.h"

namespace ml
{
// NamedValue for initializer lists

struct NamedValue
{
  ml::Path name{};
  Value value{};

  NamedValue() = default;
  NamedValue(ml::Path np, Value nv) : name(np), value(nv) {}
};

// Define a type for initializing a new PropertyTree with a list of Values.
using WithValues = const std::initializer_list<NamedValue>;

class PropertyTree
{
  Tree<Value> properties;

 public:
  PropertyTree() = default;
  PropertyTree(Tree<Value> vt) : properties(vt) {}
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

  std::vector<unsigned char> propertyTreeToBinary() { return valueTreeToBinary(properties); }
  PropertyTree binaryToPropertyTree(const std::vector<unsigned char>& binaryData)
  {
    return PropertyTree(binaryToValueTree(binaryData));
  }

  void overwrite(const PropertyTree& other)
  {
    for (auto it = other.properties.begin(); it != other.properties.end(); ++it)
    {
      setProperty(it.getCurrentNodeName(), *it);
    }
  }

  void dump() { properties.dump(); }
};

}  // namespace ml
