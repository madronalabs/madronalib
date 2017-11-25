//
//  resourceMapTest.cpp
//  madronalib
//
//  Created by Randy Jones on 9/23/15.
//
//

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <thread>

#include "catch.hpp"
#include "../include/madronalib.h"
#include "MLTextUtils.h"
#include "MLResourceMap.h"

using namespace ml;

TEST_CASE("madronalib/core/resourceMap", "[resourceMap]")
{
	textUtils::NameMaker namer;
	const int numTestWords = 100;
	const int mapSize = 100;
	std::chrono::time_point<std::chrono::system_clock> start, end;	
	std::chrono::duration<double> elapsed;
	
	// make random paths out of nonsense symbols
	auto testWords = ml::textUtils::vectorOfNonsenseSymbols( numTestWords );
	std::vector< Path > pathsVector;
	ml::textUtils::NameMaker endNamer;
	
	ml::RandomSource randSource;
	
	// make vector of test paths with mostly leaf nodes, somewhat mirroring typical use
	for(int i = 0; i < mapSize; ++i)
	{
		int pathDepth = ((ml::rand32() >> 16) & 0x07) + 2;
		int leaves = ((ml::rand32() >> 16) & 0x07) + 1;
		
		Path testPath;
		for(int p=0; p<pathDepth - 1; ++p)
		{			
			// 8 possible symbols per level
			int symbolIdx = (((ml::rand32() >> 16)&0x07) + 8*p) % numTestWords;
			testPath.addSymbol(testWords[symbolIdx]);
		}
		
		for(int j=0; j<leaves; ++j)
		{
			// make resource path with unique end so paths are never duplicates
			Symbol leafName = testWords[(ml::rand32() >> 16) % numTestWords] + endNamer.nextName();
			Path newPath = testPath;
			newPath.addSymbol(leafName);
			pathsVector.push_back(newPath);
		}
	}
	
	ResourceMap< int > numberMap;
	
	// time set nodes tree
	start = std::chrono::system_clock::now();	
	bool problem = false;
	for(int i=1; i < mapSize; ++i)
	{
		numberMap.addValue(pathsVector[i], i);
	}

	for(int i=1; i < mapSize; ++i)
	{
		int v = numberMap.findValue(pathsVector[i]);
		if(v != i)
		{
			problem = true;			
			break;
		}
	}
	REQUIRE(!problem);
	
	int bigLeafSum = 0;
	int maxDepth = 0;
	const int correctBigLeafSum = 4950;
	const int correctMaxDepth = 8;
	
	for(auto it = numberMap.begin(); it != numberMap.end(); it++)
	{
		if(it.nodeHasValue())
		{		
			bigLeafSum += it->getValue();
		}
		if(it.getDepth() > maxDepth)
		{
			maxDepth = it.getDepth();
		}
	}
	
	REQUIRE(bigLeafSum == correctBigLeafSum);
	REQUIRE(maxDepth == correctMaxDepth);
	
	end = std::chrono::system_clock::now();
	elapsed = end-start;	
	std::cout << "resource map elapsed time: " << elapsed.count() << "s\n";
	
	// with a ResourceMap< int, std::less<Symbol> > , (default sorting class), the order of map keys depends on 
	// the sorted order of symbols, which is just their creation order.
	theSymbolTable().clear();
	ResourceMap< int > a;

	a.addValue("case/sensitive/a", 1);
	a.addValue("case/sensitive/b", 1);
	a.addValue("case/sensitive/B", 1);
	a.addValue("case/sensitive/c", 1);
	a.addValue("this/is/a/test", 5); // note that non-leaf nodes may have values
	a.addValue("this/is/a/test/jam", 5);
	a.addValue("this/was/an/test", 10);
	a.addValue("this/was/another/test", 10);
	a.addValue("this/was/happy", 100);
	a.addValue("this/was/happy", 10); // duplicate addresses are overwritten
	a.addValue("you/are/my/sunshine", 10);
	a.addValue("you/are/carl's/sunshine", 10);
	a.addValue("you/are/carl's/jr/jam", 10);
	int leafSum = 0;
	const int correctLeafSum = 74;
	
	for(auto it = a.begin(); it != a.end(); it++)
	{
		if(it.nodeHasValue())
		{		
			leafSum += it->getValue();
		}
	}
	REQUIRE(leafSum == correctLeafSum);
	
	a.dump();
	
	// pass a different functor to get lexicographical sorting. 
	
	

}

