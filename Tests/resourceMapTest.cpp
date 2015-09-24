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
	MLResourceMap< int > numberMap;
	MLNameMaker namer;
	const int testSymbols = 100;
	const int mapSize = 1000;
	std::chrono::time_point<std::chrono::system_clock> start, end;	
	std::chrono::duration<double> elapsed;
	
	// make random paths out of nonsense symbols
	auto symbols = MLStringUtils::vectorOfNonsenseWords( testSymbols );
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
		// some paths are duplicates so some numbers will be overwritten
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
	end = std::chrono::system_clock::now();
	elapsed = end-start;	
	std::cout << "resource map elapsed time: " << elapsed.count() << "s\n";
}




