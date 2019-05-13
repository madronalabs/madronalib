
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
    ml::Path name;

    // note: the order of the values is important for creating Values from initializer lists.
    Value newValue;
    Value oldValue;

    explicit operator bool() const
    {
      return (oldValue || newValue);
    }
  };

  // Define a type for initializing a new object with a list of ValueChanges.
  using withValues = std::initializer_list<ValueChange>;

} // namespace ml
