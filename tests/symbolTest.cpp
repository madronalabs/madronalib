//
//  symbolTable
//


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "../include/madronalib.h"

const char letters[24] = "abcdefghijklmnopqrstuvw";


int main(int argc, char *argv[]) 
{
	const int kTableSize = 1000;	
	const int kTestLength = 1000000;

	// test: is std::unordered_map<MLSymbol, float> faster than std::unordered_map<std::string, float> in a typical use?
	std::map<MLSymbol, float> testMap1;
	std::map<std::string, float> testMap2;
	
	// make test maps and symbol / string lists
	MLNameMaker nameMaker;
	std::vector<MLSymbol> symbolDict;
	std::vector<std::string> stringDict;
	std::vector<const char *> charDict;
	
	int p = 0;
	for(int i=0; i<kTableSize; ++i)
	{
		// make a string
		std::string newString;
		int stringType = 0;
		if(stringType)
		{		
			newString = nameMaker.nextNameAsString();
		}
		else
		{
			int length = 3 + (p%8);
			for(int j=0; j<length; ++j)
			{
				p += (i*j + 1);
				p += i%37;
				p += j%23;
				p = abs(p);
				newString += letters[p % 22];
			}		
		}
		 
		stringDict.push_back(newString);
		
		// add it to symbol table
		MLSymbol newSym(newString.c_str());
		symbolDict.push_back(newSym);
		
		// add an entry to each map
		float val = i;
		testMap1[newSym] = val;
		testMap2[newString] = val;
	}
	
	// make char dict after string dict is complete, otherwise ptrs may change!
	for(int i=0; i<stringDict.size(); ++i)
	{
		charDict.push_back(stringDict[i].c_str());
	}
	
	debug() << "table size: " << theSymbolTable().getSize();
		
	theSymbolTable().dump();
//	theSymbolTable().audit();
	
	std::chrono::time_point<std::chrono::system_clock> start, end;
	std::chrono::duration<double> elapsed_seconds;
	std::time_t end_time;
	float sum;
	int idx;

if(0)
{
	// -------------------------------------
	// lookup from existing std::strings
   	start = std::chrono::system_clock::now();
   	sum = 0.f;
	idx = 0;
	for(int i=0; i<kTestLength; ++i)
	{
		if(++idx >= kTableSize) idx = 0;	
		sum += testMap2[stringDict[idx]];
	}	
	debug() << "\nSUM [std::string]: " << sum << "\n";
	end = std::chrono::system_clock::now();
    elapsed_seconds = end-start;
    end_time = std::chrono::system_clock::to_time_t(end);
    std::cout << "existing strings, elapsed time: " << elapsed_seconds.count() << "s\n";
	// -------------------------------------

	
	// -------------------------------------
	// lookup from existing MLSymbols
	// begin time
	start = std::chrono::system_clock::now();
	sum = 0.f;
	idx = 0;
	for(int i=0; i<kTestLength; ++i)
	{
		if(++idx >= kTableSize) idx = 0;	
		sum += testMap1[symbolDict[idx]];
	}
	debug() << "\nSUM [MLSymbol]: " << sum << "\n";
	end = std::chrono::system_clock::now();
	elapsed_seconds = end-start;
	end_time = std::chrono::system_clock::to_time_t(end);
	std::cout << "existing symbols, elapsed time: " << elapsed_seconds.count() << "s\n";
	// -------------------------------------

	
	// -------------------------------------
	// lookup from new MLSymbols made from char * 
	// begin time
	start = std::chrono::system_clock::now();
	sum = 0.f;
	idx = 0;
	for(int i=0; i<kTestLength; ++i)
	{
		if(++idx >= kTableSize) idx = 0;	
		sum += testMap1[charDict[idx]];
	}
	debug() << "\nSUM [MLSymbol]: " << sum << "\n";
	end = std::chrono::system_clock::now();
	elapsed_seconds = end-start;
	std::cout << "newly made symbols, elapsed time: " << elapsed_seconds.count() << "s\n";
	// -------------------------------------
}
	MLSymbol nullSym;
	if(!nullSym)
	{
		debug() << "null symbol is null.\n";
	}
	
	// -------------------------------------
	// test using a std::map while adding symbols
//	theSymbolTable().clear();
	MLNameMaker namer;
	std::map<MLSymbol, int> numbersMap;
	int strIdx = 0;
	for(int i=0; i<10; ++i)
	{
		MLSymbol newName;
		const MLSymbol name = stringDict[strIdx++];
		if(name)
		{
			if(numbersMap.find(name) != numbersMap.end())
			{
				debug() << name << " is taken?!\n";
			}
			else
			{
				debug() << "adding " << name << "...\n"; 
				newName = name;
			}
			MLSymbol name2 = stringDict[strIdx++];
			MLSymbol name3 = stringDict[strIdx++];
			MLSymbol name4 = stringDict[strIdx++];
			debug() << name2 << " " << name3 << " " << name4 << "\n";
		}
		else
		{
			newName = namer.nextName();
		}
		
		if(newName)
		{
			
			numbersMap[newName] = i;
		}
	}
	
	debug() << "---------------\n";
	for(auto m : numbersMap)
	{
		debug() << m.first << " : " << m.second << "\n";
	}
		
	return 0;
}


/*
1000000 in 0.5 s
1000000 in 20000 samples
1000 in 20 samples
100 in 2 samples
1 in 0.02 samples

say we have a 16 sample buffer. processing 1000 symbol lookups over the course of our network will take 4/5 or 80% of our CPU time! 

show results with map and unordered_map for both types of keys.
 
 unordered_map<MLSymbol, float>
1000000 in 0.01 s
1000000 in 400 samples
1000 in 0.4 samples
100 in 0.04 samples
 

*/




