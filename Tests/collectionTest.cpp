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

struct CollectableInt //: public Collectable
{
  CollectableInt() : value(0){};
  CollectableInt(int v) : value(v){};
  virtual ~CollectableInt() = default;
  operator int() const { return value; }
  int value;
};

struct FancyCollectableInt : public CollectableInt
{
  FancyCollectableInt() : CollectableInt() {}
  FancyCollectableInt(int v) : CollectableInt(v) {}
  ~FancyCollectableInt() = default;
};


// CollectableInt with access to the collection it is in, from the root node
// where it it added. This lets objects manage objects in the collection under
// them.
//
// what's this good for? one example is a View class that wants to draw and manipulate
// part of an object hierarchy, but does not own the object data itself.
struct CollectableIntWithCollection : public CollectableInt
{
  CollectableIntWithCollection(ml::Collection< CollectableInt > t, int v) : CollectableInt(v), _subCollection(t) {}
  ~CollectableIntWithCollection() = default;
  ml::Collection< CollectableInt > _subCollection;
};

TEST_CASE("madronalib/core/collection", "[collection]")
{
  CollectionRoot< CollectableInt > ints;
 
  ints.add_unique< CollectableInt >("a", 3);
  ints.add_unique< CollectableInt >("j", 4);
  ints.add_unique< FancyCollectableInt >("a/b/c/d", 5);
  ints.add_unique< CollectableInt >("a/b/c/f", 6);
  ints.add_unique< CollectableInt >("k", 7);
  ints.add_unique_with_collection< CollectableIntWithCollection >("a/b/c", 42);

  REQUIRE(ints["a/b/c/d"]->value == 5);
  
  int total{0};
  forEach< CollectableInt >(ints, [&](const CollectableInt& i){ total += i.value; });
  // std::cout << "total: " << total << "\n";
  
  // get total under "a"
  int totala{0};
  auto subIntsA = getSubCollection(ints, "a");
  forEach< CollectableInt >(subIntsA, [&](const CollectableInt& i){ totala += i.value; });
  
  // same thing using inSubCollection
  int totalb{0};
  forEach< CollectableInt >(inSubCollection(ints, "a"), [&](const CollectableInt& i){ totalb += i.value; });
  
  REQUIRE(totala == totalb);
  
  int rootTotal{0};
  forEachChild< CollectableInt >(ints, [&](const CollectableInt& i){ rootTotal += i.value; });
  // std::cout << "rootTotal: " << rootTotal << "\n";
  REQUIRE(rootTotal == 14);
  
  REQUIRE(!ints["a/b/xxx"]);
  // int thisWouldCrash = (ints["a/b/xxx"]->value);
  
  // make a subcollection and do some stuff
  auto subInts = getSubCollection(ints, "a");
  REQUIRE(!subInts["a"]); // no node - can't access value
  REQUIRE(!subInts["b"]); // node but no value at node
  REQUIRE(subInts["b/c/d"]);
  int b7 = (subInts["b/c/d"]->value);
  REQUIRE(subInts["b/c/d"]->value == 5);// node exists - return value

  // set a value using the subcollecion
  REQUIRE(subInts["b/c"]);
  subInts["b/c"]->value = 44;
  REQUIRE(ints["a/b/c"]->value == 44);
  
  // add a new object using add_unique
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
  
  // dump the collection, getting the current path with each call.
  // this forEach() syntax with the optional Path* argument is weird, but concise.
  // std::cout << "collection with paths:\n";
  /*
  Path currentPath;
  forEach< CollectableInt >(ints, [&](const CollectableInt& i)
                            {
    std::cout << currentPath << ": ";
    std::cout << i.value << "\n";
  }, &currentPath);
*/
  
  REQUIRE(ints.size() == 7);
  
  // doing things with a null collection should not crash
  auto nullColl = getSubCollection(ints, "nowhere");
  int nullCount {0};
  for(auto& obj : nullColl)
  {
    nullCount++;
  }
  for(auto& obj : inSubCollection(ints, "nowhere"))
  {
    nullCount++;
  }
  REQUIRE(!nullCount);
  
  // TODO create subcollection by filter
  //  auto subInts3 = getSubCollection(ints, [](const CollectableInt& i){return i.value > 3;});
  //
  //  auto valueFilter = [&](const CollectableInt& i){return i.value > 3;};
  //  forEach< CollectableInt >(inSubCollection(ints, valueFilter), /* do something */);

}

