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

#include "MLSerialization.h"
#include "MLTextUtils.h"
#include "MLTree.h"
#include "MLValue.h"
#include "catch.hpp"
#include "madronalib.h"
#include "mldsp.h"

using namespace ml;

struct TestResource
{
  static int instances;
  TestResource(float val)
  {
    instances++;
    std::cout << " NEW TestResource (" << val << ")\n";
    data[10] = val;
  }
  ~TestResource()
  {
    instances--;
    std::cout << " DELETE TestResource (" << data[10] << ")\n";
  }

  std::array<float, 1000> data{};
};

int TestResource::instances{};

std::ostream& operator<<(std::ostream& out,
                         const std::unique_ptr<TestResource>& p)
{
  out << p->data[10];
  return out;
}

TEST_CASE("madronalib/core/tree", "[tree]")
{
  textUtils::NameMaker namer;
  const int numTestWords = 100;
  const int mapSize = 100;

  // make random paths out of nonsense symbols
  auto testWords = ml::textUtils::vectorOfNonsenseSymbols(numTestWords);
  std::vector<Path> pathsVector;
  ml::textUtils::NameMaker endNamer;

  RandomScalarSource randSource;

  // make vector of test paths with mostly leaf nodes, somewhat mirroring
  // typical use
  for (int i = 0; i < mapSize; ++i)
  {
    int pathDepth = ((randSource.getUInt32() >> 16) & 0x07) + 2;
    int leaves = ((randSource.getUInt32() >> 16) & 0x07) + 1;

    Path testPath;
    for (int p = 0; p < pathDepth - 1; ++p)
    {
      // 8 possible symbols per level
      int symbolIdx =
          (((randSource.getUInt32() >> 16) & 0x07) + 8 * p) % numTestWords;
      testPath = Path{testPath, testWords[symbolIdx]};
    }

    for (int j = 0; j < leaves; ++j)
    {
      // make resource path with unique end so paths are never duplicates
      Symbol leafName =
          testWords[(randSource.getUInt32() >> 16) % numTestWords] +
          endNamer.nextName();
      Path newPath = testPath;
      newPath = Path{newPath, leafName};
      pathsVector.push_back(newPath);
    }
  }

  // Test a pretty big tree of int values

  {
    Tree<int> numberTree;

    bool problem = false;
    for (int i = 1; i < mapSize; ++i)
    {
      numberTree.add(pathsVector[i], i);
    }

    REQUIRE(numberTree.size() == mapSize - 1);
    
    // using a const reference will prevent the Tree from being modified.
    const Tree<int>& constNumberMap(numberTree);
    // *** constNumberMap["foo"] = 2; // should not compile.

    for (int i = 1; i < mapSize; ++i)
    {
      int v = constNumberMap[pathsVector[i]];
      if (v != i)
      {
        std::cout << "problem at " << pathsVector[i] << ": expected " << i
                  << ", found " << v << "\n";
        problem = true;
        break;
      }
    }
    REQUIRE(!problem);

    int bigValueSum = 0;
    int bigValueSum2 = 0;
    size_t maxDepth = 0;
    const int correctSum = 4950;
    const int correctMaxDepth = 8;

    // use iterator explicitly to keep track of depth and add up values.
    for (auto it = numberTree.begin(); it != numberTree.end(); ++it)
    {
      bigValueSum += *it;
      if (it.getCurrentDepth() > maxDepth)
      {
        maxDepth = it.getCurrentDepth();
      }
    }

    // use range-based for to add up values
    for (auto val : numberTree)
    {
      bigValueSum2 += val;
    }

    REQUIRE(bigValueSum == correctSum);
    REQUIRE(maxDepth == correctMaxDepth);
    REQUIRE(bigValueSum2 == correctSum);
  }

  // Misc examples

  {
    // with a Tree< int, std::less<Symbol> > , (default sorting class), the
    // order of map keys depends on the sorted order of symbols, which is just
    // their creation order.  Pass a different sorting functor than the default
    // to get lexicographical or other sorting.

    theSymbolTable().clear();
    Tree<int> a;

    // note that the root node (case) has no value.
    a.add("case/sensitive/a", 1);
    a.add("case/sensitive/b", 1);
    a.add("case/sensitive/B", 1);
    a.add("case/sensitive/c", 1);

    // note that non-leaf nodes may have values
    a.add("this/is/a/test", 5);
    a.add("this/was/an/test", 10);
    a.add("this/was/another/test", 10);
    a.add("this/is/a/test/jam", 5);

    // duplicate addresses are overwritten
    a.add("this/was/happy", 100);
    a.add("this/was/happy", 10);

    a.add("you/are/my/sunshine", 10);
    a.add("you/are/carl's/sunshine", 10);
    a.add("you/are/carl's/jr/jam", 10);

    // looking up a nonexistent node should return a reference to the default
    // value
    std::cout << "sunshine:" << a["you/are/my/sunshine"] << "\n";
    std::cout << "nonexistent:" << a["you/are/here/just/to/return/a/reference"]
              << "\n";

    int leafSum = 0;
    const int correctLeafSum = 74;

    a.dump();
    std::cout << "size: " << a.size() << "\n";

    for (auto val : a)
    {
      leafSum += val;
    }
    REQUIRE(leafSum == correctLeafSum);

    REQUIRE(std::accumulate(a.begin(), a.end(), 0) == correctLeafSum);
    
    // Example using Tree with unique_ptr< int >.
    //
    Tree< std::unique_ptr< int > > intPtrTree;
    intPtrTree["harry"] = ml::make_unique< int >(3);
    intPtrTree["mark"] = ml::make_unique< int >(0);

    REQUIRE(intPtrTree["mark"]);
    REQUIRE(*intPtrTree["mark"] == 0);
    
    REQUIRE(!intPtrTree["john"]);
    // *** REQUIRE(*intPtrTree["john"] == 0); // no value, so this will crash
  }

  // Tree example using unique_ptr to manage heavyweight objects.
  {
    // note that we want to stay compatible with C++11, which lacks
    // make_unique(), so our own ml::make_unique() is defined in MLValue.h

    Tree<std::unique_ptr<TestResource> > heavies;

    // an existing unique_ptr cannot be assigned because its copy assignment
    // operator is implicitly deleted
    const auto r = make_unique<TestResource>(2);
    // *** heavies["x"] = r;
    // *** heavies.add("nodes/in/path",  r);

    // we can check to see if an object exists without making one
    REQUIRE(!treeNodeExists(heavies, "x"));

    // either add() or operator[] can be used to assign a new unique_ptr
    heavies.add("x", make_unique<TestResource>(8));
    heavies["x"] = make_unique<TestResource>(10);
    REQUIRE(treeNodeExists(heavies, "x"));

    // shorthand courtesy of unique_ptr operator bool
    REQUIRE(heavies["x"]);
    
    // an empty unique_ptr is added here, but not the heavy object
    // it would contain
    REQUIRE(!heavies["y"]);
 
    // when overwriting nodes, unique_ptr handles deletion
    heavies.add("duplicate/nodes/in/path", make_unique<TestResource>(4));
    heavies.add("duplicate/nodes/in/path", make_unique<TestResource>(6));

    // note, this intentionally generates a compile-time error when using
    // unique_ptrs
    // *** auto failedLookup = heavies["nowhere/in/path"];

    // instead, a reference must be used.
    // if no value is found, a new node with default value is added and
    // returned. in this case, that value is a unique_ptr to null. so failed
    // lookups don't result in a new resource being made.
    auto& failedLookup = heavies["nowhere/in/path"];
    REQUIRE(failedLookup.get() == nullptr);
    REQUIRE(!failedLookup);  // shorthand courtesy of unique_ptr operator bool

    // overwrite dataheavies["x"]
    heavies["x"]->data[10] = 100;

    // dump() works because we have declared operator<< (std::ostream& out,
    // const std::unique_ptr< TestResource >& p) above
    heavies.dump();
  }
  REQUIRE(TestResource::instances == 0);

  // Value tree tests

  Tree<Value> properties;
  properties.add("size", "big");
  properties.add("shape", "square");
  properties.add("corners", 4);

  // add 1D matrices
  properties.add("melodies/1", {1, 2, 3, 4, 5, 6, 7});
  properties.add("melodies/2", {8, 7, 6, 5, 4, 3, 2});

  // add a 3D matrix
  Matrix melody3(4, 5, 2);
  melody3.fill(9.f);
  properties.add("melodies/3", melody3);

  properties.add("melodies/3/dummy", 4);

  // when a property does not exist, operator[] adds a default object
  // and the reference it returns can be assigned a value
  REQUIRE(!treeNodeExists(properties, "x"));
  properties["x"] = 24;
  REQUIRE(treeNodeExists(properties, "x"));

  // failed lookup returns a null Value
  auto failedLookup = properties["nowhere/in/path"];
  REQUIRE(failedLookup == Value());

  // a tree converted to binary and back should result in the original value
  auto b = valueTreeToBinary(properties);
  auto b2 = valueTreeToBinary(binaryToValueTree(b));
  REQUIRE(b == b2);

  std::vector<Value> melodies;
  for (auto it = properties.begin(); it != properties.end(); ++it)
  {
    const Path p = it.getCurrentNodePath();
    if (butLast(p) == Path("melodies"))
    {
      melodies.push_back(*it);
    }
  }
  for (auto m : melodies)
  {
    std::cout << "    " << m << "\n";
  }
  REQUIRE(melodies.size() == 3);

  //  Empty Tree test
  Tree<Value> emptyTree;
  int count{0};
  for (auto it = emptyTree.begin(); it != emptyTree.end(); ++it)
  {
    count++;
  }
  REQUIRE(count == 0);
  emptyTree["this/is/a/test"] = Value{2, 3, 4, 5};
  for (auto it = emptyTree.begin(); it != emptyTree.end(); ++it)
  {
    count++;
  }
  REQUIRE(count == 1);
  
  // Tree of bare floats test
  Tree< float > floatTree;
  REQUIRE(floatTree["purple"] == 0.f);
  floatTree["pink"] = 1.f;
  REQUIRE(floatTree["pink"] == 1.f);

}

TEST_CASE("madronalib/core/serialization", "[serialization]")
{
  // serialization sample code.
  // TODO test float -> text -> float conversion

  NoiseGen n;
  for (int i = 0; i < 80; ++i)
  {
    float v = (1.0f + fabs(n.getSample()) * 9.f) * powf(10.f, i - 40 + 0.f) *
              (i & 1 ? -1 : 1);
    auto t = textUtils::floatNumberToText(v, 15);
    auto f = textUtils::textToFloatNumber(t);
    //std::cout << std::setprecision(10) << v << " -> \"" << t << "\" -> " << f << "\n";
  }

  const float maxFloat = std::numeric_limits<float>::max();
  const float minFloat = std::numeric_limits<float>::min();
  const float nanFloat = std::numeric_limits<float>::quiet_NaN();
  std::vector<float> vf {nanFloat, maxFloat*10.f, maxFloat, maxFloat/10.f,
  10000001, 32768, 10000, 100, 10.0f, 1.00001f, 1, 0.25f, 0.1f, 0.1250001f, 0.125f,
  0.1249999f, 0.f, 3.004e-02f, 3.004e-07f, minFloat};
  for(auto v : vf)
  {
  auto t = textUtils::floatNumberToText(v, 10);
  auto f = textUtils::textToFloatNumber(t);
  // std::cout << std::setprecision(10) << v << " -> \"" << t << "\" -> " << f << "\n";
  }
}

