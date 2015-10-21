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
#include "MLStringUtils.h"
#include "MLResourceMap.h"

TEST_CASE("madronalib/core/resourceMap", "[resourceMap]")
{
	MLResourceMap< std::string, int > numberMap;
	MLNameMaker namer;
	const int testSymbols = 100;
	const int mapSize = 100;
	std::chrono::time_point<std::chrono::system_clock> start, end;	
	std::chrono::duration<double> elapsed;
	
	// make random paths out of nonsense symbols
	auto symbols = ml::stringUtils::vectorOfNonsenseWords( testSymbols );
	std::vector<std::string> paths;
	MLNameMaker endNamer;
	
	// make test tree with mostly leaf nodes, somewhat mirroring typical use
	for(int i = 0; i < mapSize; ++i)
	{
		int pathDepth = ((MLRand32() >> 16) & 0x07) + 2;
		int leaves = ((MLRand32() >> 16) & 0x07) + 1;
		std::string pathStr;
		for(int p=0; p<pathDepth - 1; ++p)
		{			
			// 8 possible symbols per level
			int symbolIdx = (((MLRand32() >> 16)&0x07) + 8*p) % testSymbols;
			pathStr = pathStr + symbols[symbolIdx];
			if(p < pathDepth - 1)
			{
				pathStr = pathStr + "/";
			}
		}
		
		for(int j=0; j<leaves; ++j)
		{
			// make resource name with unique end so paths are never duplicates
			std::string leafName = symbols[(MLRand32() >> 16) % testSymbols] + endNamer.nextName().getString();
			paths.push_back(pathStr + leafName);
		}
	}
	
	// time set nodes tree
	start = std::chrono::system_clock::now();	
	bool problem = false;
	for(int i=0; i < mapSize; ++i)
	{
		numberMap.addValue(paths[i], i);
	}
	for(int i=0; i < mapSize; ++i)
	{
		int v = numberMap.findValue(paths[i]);
		if(v != i)
		{
			problem = true;			
			break;
		}
	}
	REQUIRE(!problem);
	
	int bigLeafSum = 0;
	int maxDepth = 8;
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
	
	// this default comparison is not case sensitive: case/B and case/b are addresses of the same node. 
	// with a MLResourceMap< MLSymbol, int, std::less<MLSymbol> > , the order of map depends on 
	// the sorted order of symbols, which is just their creation order.
	MLSymbol h("this");
	MLSymbol t("case");	
	MLResourceMap< MLSymbol, int > a;

	a.addValue("case/sensitive/a", 1);
	a.addValue("case/sensitive/b", 1);
	a.addValue("case/sensitive/c", 1);
	a.addValue("case/sensitive/B", 1); 
	a.addValue("this/is/a/test", 10);
	a.addValue("this/was/an/test", 10);
	a.addNode("this/was/another");
	a.addValue("this/was/another/test", 10);
	a.addValue("this/was/happy", 10);
	a.addNode("this/is/an/empty/directory");
	a.addValue("you/are/my/sunshine", 10);
	a.addValue("you/are/carl's/sunshine", 10);
	a.addValue("you/are/carl's/jr/jam", 10);
	a.addNode("you/are/carl's/jr");
	int leafSum = 0;
	const int correctLeafSum = 73;
	
	for(auto it = a.begin(); it != a.end(); it++)
	{
		if(it.nodeHasValue())
		{		
			leafSum += it->getValue();
		}
	}
	REQUIRE(leafSum == correctLeafSum);
	
}




