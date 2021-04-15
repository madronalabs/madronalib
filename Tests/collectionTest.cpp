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

#include "MLCollection.h"
#include "catch.hpp"

using namespace ml;

struct CollectableInt : public Collectable
{
  CollectableInt() : value(0){};
  CollectableInt(int v) : value(v){};
  virtual ~CollectableInt() = default;
  operator int() const { return value; }
  virtual Value respond(Message m) { std::cout << " " << value << " "; return false; }
  int value;
};

struct FancyCollectableInt : public CollectableInt
{
  FancyCollectableInt() : CollectableInt() {}
  FancyCollectableInt(int v) : CollectableInt(v) {}
  Value respond(Message m) override { std::cout << " ***" << value << "*** "; return false; }
};

TEST_CASE("madronalib/core/collection", "[collection]")
{
  CollectionRoot< CollectableInt > ints;
  
  ints.add_unique< CollectableInt >("a", 4);
  ints.add_unique< FancyCollectableInt >("a/b/c", 5);
  ints.add_unique< CollectableInt >("a/b/d", 6);
  
  REQUIRE(ints["a/b/c"]->value == 5);
  REQUIRE(ints["a/b/d"]->value == 6);
  
  std::cout << "messages: ";
  forEach< CollectableInt >(ints, [&](CollectableInt& i){ sendMessage(i, {"ack"}); });
  std::cout << "\n";

  int total{0};
  forEachConst< CollectableInt >(ints, [&](const CollectableInt& i){ total += i.value; });
  std::cout << "total: " << total << "\n";

  REQUIRE(!ints["a/b/xxx"]);
  // int thisWouldCrash = (ints["a/b/xxx"]->value);

  // make a subcollection and do some stuff
  auto subInts = getSubCollection(ints, "a");
  
  REQUIRE(!subInts["a"]); // no node - can't access value
  REQUIRE(!subInts["b"]); // node but no value at node
  REQUIRE(subInts["b/c"]);
  int b7 = (subInts["b/c"]->value);
  REQUIRE(subInts["b/c"]->value == 5);// node exists - return value


  // set a value using the subcollecion
  REQUIRE(subInts["b/c"]);
  subInts["b/c"]->value = 44;
  REQUIRE(ints["a/b/c"]->value == 44);
  
  // add a new object using subcollection
  subInts.add_unique< FancyCollectableInt >("b/the/new/guy", 99);
  REQUIRE(ints["a/b/the/new/guy"]);
  REQUIRE(ints["a/b/the/new/guy"]->value == 99);

  REQUIRE(!ints.find("a/b/xxx"));
  REQUIRE(ints.find("a/b/the/new/guy"));

  auto& newGuyPtr{ints.find("a/b/the/new/guy")};
  if(newGuyPtr)
  {
    REQUIRE(newGuyPtr->value == 99);
    
  }
  

  // TODO a const subcollection that does not let us modify the held
  // values would be handy - revisit
  
  // TODO create subcollection by test
  //  auto subInts3 = getSubCollection([](const CollectableInt& i){return i.value > 3;});

}

