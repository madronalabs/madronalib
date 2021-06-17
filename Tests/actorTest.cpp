// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <thread>
#include <unordered_map>
#include <vector>

#include "MLActor.h"
#include "MLTextUtils.h"
#include "MLTree.h"
#include "MLValue.h"
#include "catch.hpp"
#include "madronalib.h"
#include "mldsp.h"

using namespace ml;

class MyActor : public Actor
{
public:

  // Actor interface
  void handleMessage(Message m) override
  {
    
  }

};

TEST_CASE("madronalib/core/actors", "[actors]")
{
  // TODO
  MyActor a;
  
}

