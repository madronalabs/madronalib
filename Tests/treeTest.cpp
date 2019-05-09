//
//  treeTest.cpp
//  madronalib
//
//  Created by Randy Jones on 9/23/15.
//
//

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <thread>
#include <memory>
#include <numeric>

#include "catch.hpp"
#include "madronalib.h"
#include "MLTextUtils.h"
#include "MLTree.h"
#include "MLValue.h"

using namespace ml;

struct TestResource
{
  static int instances;
  TestResource(float val) { instances++; std::cout << " NEW TestResource (" << val << ")\n"; data[10] = val;}
  ~TestResource() { instances--; std::cout << " DELETE TestResource (" << data[10] << ")\n"; }

  std::array<float, 1000> data{};
};

int TestResource::instances{};

std::ostream& operator<< (std::ostream& out, const std::unique_ptr< TestResource >& p)
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
	auto testWords = ml::textUtils::vectorOfNonsenseSymbols( numTestWords );
	std::vector< Path > pathsVector;
	ml::textUtils::NameMaker endNamer;
	
	RandomScalarSource randSource;
	
	// make vector of test paths with mostly leaf nodes, somewhat mirroring typical use
	for(int i = 0; i < mapSize; ++i)
	{
		int pathDepth = ((randSource.getUInt32() >> 16) & 0x07) + 2;
		int leaves = ((randSource.getUInt32() >> 16) & 0x07) + 1;
		
		Path testPath;
		for(int p=0; p<pathDepth - 1; ++p)
		{			
			// 8 possible symbols per level
			int symbolIdx = (((randSource.getUInt32() >> 16)&0x07) + 8*p) % numTestWords;
			testPath.addSymbol(testWords[symbolIdx]);
		}
		
		for(int j=0; j<leaves; ++j)
		{
			// make resource path with unique end so paths are never duplicates
			Symbol leafName = testWords[(randSource.getUInt32() >> 16) % numTestWords] + endNamer.nextName();
			Path newPath = testPath;
			newPath.addSymbol(leafName);
			pathsVector.push_back(newPath);
		}
	}


  // Test a pretty big tree of int values

  {
    Tree< int > numberMap;

    bool problem = false;
    for(int i=1; i < mapSize; ++i)
    {
      numberMap.add(pathsVector[i], i);
    }

    // using a const reference will prevent the Tree from being modified.
    const auto& constNumberMap(numberMap);
    // *** constNumberMap["foo"] = 2;
    
    for(int i=1; i < mapSize; ++i)
    {
      int v = constNumberMap[pathsVector[i]];
      if(v != i)
      {
        std::cout << "problem at " << pathsVector[i] << ": expected " << i << ", found " << v << "\n";
        problem = true;
        break;
      }
    }
    REQUIRE(!problem);

    int bigValueSum = 0;
    int bigValueSum2 = 0;
    int maxDepth = 0;
    const int correctSum = 4950;
    const int correctMaxDepth = 8;

    // use iterator explicitly to keep track of depth and add up values.
    for(auto it = numberMap.begin(); it != numberMap.end(); ++it)
    {
      bigValueSum += *it;
      if(it.getCurrentDepth() > maxDepth)
      {
        maxDepth = it.getCurrentDepth();
      }
    }

    // use range-based for to add up values
    for(auto val : numberMap)
    {
      bigValueSum2 += val;
    }

    REQUIRE(bigValueSum == correctSum);
    REQUIRE(maxDepth == correctMaxDepth);
    REQUIRE(bigValueSum2 == correctSum);
  }


  // Misc examples

  {
    // with a Tree< int, std::less<Symbol> > , (default sorting class), the order of map keys depends on
    // the sorted order of symbols, which is just their creation order.  Pass a different sorting functor
    // than the default to get lexicographical or other sorting.

    theSymbolTable().clear();
    Tree< int > a;

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

    // looking up a nonexistent node should return a reference to the default value
    std::cout << "sunshine:" << a["you/are/my/sunshine"] << "\n";
    std::cout << "nonexistent:" << a["you/are/here/just/to/return/a/reference"] << "\n";

    int leafSum = 0;
    const int correctLeafSum = 74;

    a.dump();

    for(auto val : a)
    {
      leafSum += val;
    }
    REQUIRE(leafSum == correctLeafSum);

    REQUIRE(std::accumulate(a.begin(), a.end(), 0) == correctLeafSum);
  }


  // Tree example using unique_ptr to manage heavyweight objects.

  {
    // note that we are compiling for C++11—our own make_unique() is defined in MLValue.h

    Tree< std::unique_ptr<TestResource> > heavies;

    // an existing unique_ptr cannot be assigned because its copy assignment operator is implicitly deleted
    const auto r = make_unique< TestResource >(2);
    // *** heavies["x"] = r;
    // *** heavies.add("nodes/in/path",  r);

    // we can check to see if an object exists without making one
    REQUIRE(heavies.valueExists("x") == false);

    // either add() or operator[] can be used to assign a new unique_ptr
    heavies.add("x", make_unique< TestResource >(8) );
    heavies["x"] = make_unique< TestResource >(10);
    REQUIRE(heavies.valueExists("x") == true);
    REQUIRE(heavies["x"]); // shorthand courtesy of unique_ptr operator bool

    // when overwriting nodes, unique_ptr handles deletion
    heavies.add("duplicate/nodes/in/path", make_unique< TestResource >(4) );
    heavies.add("duplicate/nodes/in/path", make_unique< TestResource >(6) );

    // note, this intentionally generates a compile-time error when using unique_ptrs
    // *** auto failedLookup = heavies["nowhere/in/path"];

    // instead, a reference must be used.
    // if no value is found, a new node with default value is added and returned.
    // in this case, that value is a unique_ptr to null. so failed lookups don't result in a new resource being made.
    auto& failedLookup = heavies["nowhere/in/path"];
    REQUIRE(failedLookup.get() == nullptr);
    REQUIRE(!failedLookup); // shorthand courtesy of unique_ptr operator bool

    // overwrite data
    heavies["x"]->data[10] = 100;

    // dump() works because we have declared operator<< (std::ostream& out, const std::unique_ptr< TestResource >& p) above
    heavies.dump();
  }
  REQUIRE(TestResource::instances == 0);


  // Value tree example

  Tree< Value > properties;
  properties.add("size", "big");
  properties.add("shape", "square");
  properties.add("corners", 4);
  properties.add("melody/1", {0, 4, 3, 5, 3, 4, 2} );
  properties.add("melody/2", {3, 4, 3, 5, 3, 4, 2} );

  // when a property does not exist, operator[] adds a default object
  // and the reference it returns can be assigned a value
  REQUIRE(!properties.valueExists("x"));
  properties["x"] = 24;

  // looking up a Value directly is fine
  auto failedLookup = properties["nowhere/in/path"];
  REQUIRE(failedLookup == Value());

  // use iterator explicitly to get property names / types.
  for(auto it = properties.begin(); it != properties.end(); ++it)
  {
    std::cout << it.getCurrentNodePath() << " = (" << (*it).getTypeAsSymbol() << ") " << *it << "\n";
  }


  //  Empty Tree test
  Tree< Value > emptyTree;
  int count{0};
  for(auto it = emptyTree.begin(); it != emptyTree.end(); ++it)
  {
    count++;
  }
  REQUIRE(count == 0);
  emptyTree["this/is/a/test"] = Value{2, 3, 4, 5};
  for(auto it = emptyTree.begin(); it != emptyTree.end(); ++it)
  {
    count++;
  }
  REQUIRE(count == 1);


}



