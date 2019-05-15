
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <string>
#include <list>
#include <map>

#include "MLMatrix.h"
#include "MLSymbol.h"
#include "MLText.h"
#include "MLPath.h"

namespace ml
{

  // utilities

  struct ValueChange
  {
    // the path to a change value in a tree
    ml::Path name{};

    // note: the order of the members is important for creating Values from initializer lists.
    // value after the change
    Value newValue{};

    // value before the change
    Value oldValue{};

    bool startGesture{false};
    bool endGesture{false};

    ValueChange() = default;
    ValueChange(ml::Path np, Value nv, Value ov = Value(), bool start = false, bool end = false) :
    name(np), newValue(nv), oldValue(ov), startGesture(start), endGesture(end) {}

    explicit operator bool() const
    {
      return (oldValue || newValue);
    }
  };

  // Define a type for initializing a new object with a list of ValueChanges.
  using withValues = std::initializer_list<ValueChange>;

} // namespace ml
