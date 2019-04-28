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

#include "catch.hpp"
#include "madronalib.h"
#include "MLTextUtils.h"
#include "MLTree.h"

using namespace ml;

TEST_CASE("madronalib/core/tree", "[tree]")
{
	textUtils::NameMaker namer;
	const int numTestWords = 100;
	const int mapSize = 100; // MLTEST
	
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


	Tree< int > numberMap;

  bool problem = false;
	for(int i=1; i < mapSize; ++i)
	{
		numberMap.addValue(pathsVector[i], i);
	}

  numberMap.dump();

	for(int i=1; i < mapSize; ++i)
	{
		int v = numberMap.getValue(pathsVector[i]);
		if(v != i)
		{

      std::cout << "problem at " <<pathsVector[i] << ": expected " << i << ", found " << v << "\n";
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

  // use iterator explicitly to keep track of depth.
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

  
	// with a Tree< int, std::less<Symbol> > , (default sorting class), the order of map keys depends on
  // the sorted order of symbols, which is just their creation order.  Pass a different sorting functor
  // than the default to get lexicographical or other sorting.

	theSymbolTable().clear();
	Tree< int > a;

  // note that the root node (case) has no value.
	a.addValue("case/sensitive/a", 1);
	a.addValue("case/sensitive/b", 1);
	a.addValue("case/sensitive/B", 1);
	a.addValue("case/sensitive/c", 1);

  // note that non-leaf nodes may have values
	a.addValue("this/is/a/test", 5);
	a.addValue("this/was/an/test", 10);
	a.addValue("this/was/another/test", 10);
  a.addValue("this/is/a/test/jam", 5);

  // duplicate addresses are overwritten
	a.addValue("this/was/happy", 100);
	a.addValue("this/was/happy", 10);

	a.addValue("you/are/my/sunshine", 10);
	a.addValue("you/are/carl's/sunshine", 10);
	a.addValue("you/are/carl's/jr/jam", 10);

	int leafSum = 0;
	const int correctLeafSum = 74;

  a.dump();

  for(auto val : a)
	{
    leafSum += val;
	}

  REQUIRE(leafSum == correctLeafSum);

}

