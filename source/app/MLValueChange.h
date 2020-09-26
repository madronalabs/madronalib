
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <initializer_list>
#include <iostream>

#include "MLPath.h"
#include "MLValue.h"

namespace ml
{
struct ValueChange
{
  // path referring to a value somewhere, probably in a Tree< Value >
  Path name{};

  // note: the order of the members is important for creating Values from
  // initializer lists. value after the change
  Value newValue{};

  // value before the change
  Value oldValue{};

  bool startGesture{false};
  bool endGesture{false};

  // Widget that triggered the change, if applicable
  Path triggerWidget{};

  ValueChange() = default;
  ValueChange(ml::Path np, Value nv = Value(), Value ov = Value(), bool start = false,
              bool end = false)
      : name(np), newValue(nv), oldValue(ov), startGesture(start), endGesture(end)
  {
  }

  explicit operator bool() const { return newValue ? true : false; }
};

// note: because std::vector will allocate on the fly, this implementation of
// ValueChangeList is not safe for use in audio processing threads. Given the
// intended use in editors and controllers, this seems like a reasonable
// tradeoff.
using ValueChangeList = std::vector<ValueChange>;

}  // namespace ml

inline std::ostream& operator<<(std::ostream& out, const ml::ValueChange& r)
{
  std::cout << "[" << r.name << ": " << r.oldValue << " -> " << r.newValue << "]";
  return out;
}
