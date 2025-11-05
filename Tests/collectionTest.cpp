// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "MLCollection.h"

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
  
    // TEMP
  constexpr Symbol s (12345);
  
  // Test 1: Direct hash computation
  //constexpr *uint64_t h1 = hash("test");
  //std::cout << "hash " << h1 << "\n";

  
  // Test 3: Symbol from string literal
  Symbol s3("this");
  std::cout << "this hash: " << s3.getHash() << "\n";
  std::cout << "this: " << s3 << "\n";

  std::cout << "path hash: " << Symbol("path").getHash() << "\n";
  
  // Symbol from char * (not constexpr)
  std::string testStr;
  testStr = "abc";
  Symbol s4 = runtimeSymbol(testStr.c_str());
  
  std::cout << "s4 hash: " << s4.getHash() << "\n";
  
  Path p1("this/is/abc/path");
  std::cout << p1 << "\n";
  
  Symbol thisSym = p1.getElement(0);
  std::cout << "this hash: " << thisSym.getHash() << "\n";
  Symbol pathSym = p1.getElement(3);
  std::cout << "path hash: " << pathSym.getHash() << "\n";

  
  
  std::string str2("this/is/another/path");
  auto p2 = runtimePath(str2.c_str());
  std::cout << p2 << "\n";
  
  
  
  Symbol foo("foo");
  Symbol bar("bar");
  
  std::cout << "foo: " << foo << "\n";
  std::cout << "bar: " << bar << "\n";

  
  Symbol fooPlusBarSym(foo + bar);
  std::cout << "foo + bar: " << fooPlusBarSym << "\n";
  std::cout << "foo + bar hash: " << fooPlusBarSym.getHash() << "\n";
  
  Symbol foobarSym("foobar");
  std::cout << "foobar: " << foobarSym << "\n";
  std::cout << "foobar hash: " << foobarSym.getHash() << "\n";

  

            
  
  // Test 4: Check if the issue is the array reference
  //constexpr const char* str = "abc";
  //constexpr Symbol s4(str);  // Does this work? (probably not - different overload)
  
  
  /*
   constexpr Symbol s1("abc");
   constexpr Symbol s2("a");
   */
  
  const char * c5("abc");
  auto h5 = fnv1aSubstring(c5, strlen(c5));
  std::cout << "s5 hash: " << h5 << "\n";
  
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

