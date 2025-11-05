// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "madronalib.h"

using namespace ml;

struct TestResource
{
  static int instances;
  TestResource(float val)
  {
    instances++;
    data[10] = val;
  }
  ~TestResource()
  {
    instances--;
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
  auto testWords = ml::textUtils::makeVectorOfNonsenseSymbols(numTestWords);
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
      testPath = Path{testPath, runtimePath(testWords[symbolIdx].getUTF8Ptr())};
    }

    for (int j = 0; j < leaves; ++j)
    {
      // make resource path with unique end so paths are never duplicates
      Symbol leafName =
          testWords[(randSource.getUInt32() >> 16) % numTestWords] +
        runtimeSymbol(endNamer.nextName());
      Path newPath = testPath;
      newPath = Path{newPath, runtimePath(leafName.getUTF8Ptr())};
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
        // std::cout << "problem at " << pathsVector[i] << ": expected " << i << ", found " << v << "\n";
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
    // their hash order.  Pass a different sorting functor than the default
    // to get lexicographical or other sorting.

    Tree< int > a;

    // note that the root node (case) has no value.
    a.add(Path("case/sensitive/a"), 1);
    a.add("case/sensitive/b", 1);
    a.add("case/sensitive/B", 1);
    a.add("case/sensitive/c", 1);

    // note that non-leaf nodes may have values
    a.add("this/is/a/test", 5);
    a.add("this/is/a/test/jam", 5);
    a.add("this/was/an/test", 10);
    a.add("this/was/another/test", 10);
    a.add("this/is/a/super/duper/test", 1);
    a.add("this/is/a/super/duper/cosmic/jam", 5);

    // duplicate addresses are overwritten
    a.add("this/was/happy", 100);
    a.add("this/was/happy", 10);

    a.add("you/are/my/sunshine", 10);
    a.add("you/are/carl's/sunshine", 10);
    a.add("you/are/carl's/jr/jam", 10);

    // looking up a nonexistent node should return a reference to the default value
    REQUIRE(a["you/are/my/sunshine"] == 10);
    
    // note: this creates a new node with the default value!
    REQUIRE(a["this/path/does/not/have/a/value"] == 0);
    
    int leafSum = 0;
    const int correctLeafSum = 80;
    
    //std::cout << "nodes: " << a.countNodes() << "\n";
    
    for (auto val : a)
    {
      leafSum += val;
    }
    REQUIRE(leafSum == correctLeafSum);

    REQUIRE(std::accumulate(a.begin(), a.end(), 0) == correctLeafSum);
    
    // copy by value
    auto a2 = a;
    REQUIRE(std::accumulate(a2.begin(), a2.end(), 0) == correctLeafSum);
    REQUIRE(&a != &a2);

    // Example using Tree with unique_ptr< int >.
    //
    Tree< std::unique_ptr< int > > intPtrTree;
    intPtrTree["harry"] = std::make_unique< int >(3);
    intPtrTree["mark"] = std::make_unique< int >(0);

    REQUIRE(intPtrTree["mark"]);
    REQUIRE(*intPtrTree["mark"] == 0);
    
    REQUIRE(!intPtrTree["john"]);
    // *** REQUIRE(*intPtrTree["john"] == 0); // no value, so this will crash
    
    // copy by value will not compile with unique_ptr values
    // *** auto intPtrTreeB = intPtrTree;
    
    // iterate just over children. is node has no chlidren, nothing should be called.
    int sumOfChildren;
    sumOfChildren = 0;
    auto iterator = a.begin();
    iterator.setCurrentPath("case/sensitive");
    for(iterator.firstChild(); iterator.hasMoreChildren(); iterator.nextChild())
    {
      if(iterator.currentNodeHasValue())
      {
        sumOfChildren += *iterator;
      }
    }
    REQUIRE(sumOfChildren == 4);
    
    sumOfChildren = 0;
    iterator.setCurrentPath("this/is/a/test");
    for(iterator.firstChild(); iterator.hasMoreChildren(); iterator.nextChild())
    {
      if(iterator.currentNodeHasValue())
      {
        sumOfChildren += *iterator;
      }
    }
    REQUIRE(sumOfChildren == 5);
    
    sumOfChildren = 0;
    iterator.setCurrentPath("this/is");
    for(iterator.firstChild(); iterator.hasMoreChildren(); iterator.nextChild())
    {
      if(iterator.currentNodeHasValue())
      {
        sumOfChildren += *iterator;
      }
    }
    REQUIRE(sumOfChildren == 0);
    
    // try adding children at root
    a.add("peter", 1);
    a.add("paul", 1);
    a.add("mary", 1);
    sumOfChildren = 0;
    iterator.setCurrentPath(Path());
    for(iterator.firstChild(); iterator.hasMoreChildren(); iterator.nextChild())
    {
      if(iterator.currentNodeHasValue())
      {
        sumOfChildren += *iterator;
      }
    }
    REQUIRE(sumOfChildren == 3);
  }

  // Tree example using unique_ptr to manage heavyweight objects.
  {
    Tree<std::unique_ptr<TestResource> > heavies;

    // an existing unique_ptr cannot be assigned because its copy assignment
    // operator is implicitly deleted
    const auto r = std::make_unique<TestResource>(2);
    // *** heavies["x"] = r;
    // *** heavies.add("nodes/in/path",  r);

    // we can check to see if an object exists without making one
    REQUIRE(!heavies.getNode("x"));


    // either add() or operator[] can be used to assign a new unique_ptr
    heavies.add("x", std::make_unique<TestResource>(8));
    heavies["x"] = std::make_unique<TestResource>(10);

    // shorthand courtesy of unique_ptr operator bool
    REQUIRE(heavies["x"]);
    
    // an empty unique_ptr is added here, but not the heavy object
    // it would contain
    REQUIRE(!heavies["y"]);
 
    // when overwriting nodes, unique_ptr handles deletion
    heavies.add("duplicate/nodes/in/path", std::make_unique<TestResource>(4));
    heavies.add("duplicate/nodes/in/path", std::make_unique<TestResource>(6));

    // note, this intentionally generates a compile-time error when using
    // unique_ptrs
    // *** auto failedLookup = heavies["nowhere/in/path"];
    
    // copying unique_ptr is not allowed.
    // *** auto heaviesB = heavies;

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
    // heavies.dump();
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

  // when a property does not exist, operator[] adds a default object
  // and the reference it returns can be assigned a value
  REQUIRE(!properties.getNode("x"));
  properties["x"] = 24;
  REQUIRE(properties.getNode("x"));

  // failed lookup returns a null Value
  auto failedLookup = properties["nowhere/in/path"];
  REQUIRE(failedLookup == Value());
  
  // we have lightweight objects, so deep copy is OK
  auto propertiesB = properties;
  REQUIRE (propertiesB == properties);
  propertiesB["x"] = 25;
  REQUIRE (propertiesB != properties);

  std::vector< Value > melodies;
  for (auto it = properties.begin(); it != properties.end(); ++it)
  {
    const Path p = it.getCurrentPath();
    if (butLast(p) == Path("melodies"))
    {
      melodies.push_back(*it);
    }
  }

  //  Empty Tree test
  Tree< Value > emptyTree;
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

TEST_CASE("madronalib/core/textutils", "[textutils]")
{
  NoiseGen n;
  constexpr int precision = 5;
  
  // Within the range of exponents (10^-34 -- 10^34) the maximum error of
  // float -> text -> float conversion should be as tested below. outside
  // the range the errors get bigger.
  
  // Test a bunch of random numbers covering the range.
  int errors{0};
  for (int i = 7; i < 75; ++i)
  {
    float v = (1.0f + fabs(n.getSample())*9.f) * powf(10.f, i - 40)*(i&1 ? -1 : 1);
    auto t = textUtils::floatNumberToText(v, precision);
    auto f = textUtils::textToFloatNumber(t);
    float error = fabs(f - v);
    bool isExpNotation = textUtils::findFirst(t, 'e') >= 0;
    float maxError = isExpNotation ? fabs(v*powf(10.f, -precision)) : powf(10.f, -4.f);
    if(error > maxError) errors++;
  }
  REQUIRE(!errors);
  
  // test some edge cases.
  const float infFloat = std::numeric_limits<float>::infinity();
  const float nanFloat = std::numeric_limits<float>::quiet_NaN();
  std::vector< float > vf {infFloat, nanFloat, 10000001, 32768, 10000, 100, 99.99999f, 10.0f,
    9.99999f, 9.99995f, 1.00001f, 1, 0.25f, 0.1f, 9.999999e-9f, 1.11111e-10f};
  for(auto v : vf)
  {
    auto t = textUtils::floatNumberToText(v, precision);
    auto f = textUtils::textToFloatNumber(t);
    float error = fabs(f - v);
    bool isExpNotation = textUtils::findFirst(t, 'e') >= 0;
    float maxError = isExpNotation ? fabs(v*powf(10.f, -precision)) : powf(10.f, -4.f);
    if(error > maxError) errors++;
  }
  REQUIRE(!errors);
}
