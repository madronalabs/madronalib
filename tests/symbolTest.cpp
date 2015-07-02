
//
//  symbolTest
//

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <thread>

#include "../include/madronalib.h"

const char letters[24] = "abcdefghjklmnopqrstuvw";

void threadTest(int threadID)
{
	MLNameMaker namer;
	for(int i=0; i<100; ++i)
	{
		MLSymbol sym(namer.nextNameAsString());
		std::this_thread::yield();
	}
}

int main(int argc, char *argv[]) 
{
	const int kTableSize = 100;	
	const int kTestLength = 1000000;

	// main maps for testing
	std::map<MLSymbol, float> testMap1;
	std::map<std::string, float> testMap2;

	// make dictionaries of symbols, strings and chars for testing
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
			// procedural gibberish
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

	std::chrono::time_point<std::chrono::system_clock> start, end;
	std::chrono::duration<double> elapsed_seconds;
	std::time_t end_time;
	double a, b, c, d;
	int idx;

	// -------------------------------------
	// lookup from existing std::strings
	start = std::chrono::system_clock::now();
	a = 0.f;
	idx = 0;
	for(int i=0; i<kTestLength; ++i)
	{
		if(++idx >= kTableSize) idx = 0;	
		a += testMap2[stringDict[idx]];
	}	
	debug() << "\nSUM [std::string]: " << a << "\n";
	end = std::chrono::system_clock::now();
	elapsed_seconds = end-start;
	end_time = std::chrono::system_clock::to_time_t(end);
	std::cout << "existing strings, elapsed time: " << elapsed_seconds.count() << "s\n";
	// -------------------------------------
	
	// -------------------------------------
	// lookup from newly made std::strings
	start = std::chrono::system_clock::now();
	b = 0.f;
	idx = 0;
	for(int i=0; i<kTestLength; ++i)
	{
		if(++idx >= kTableSize) idx = 0;	
		b += testMap2[std::string(charDict[idx])];
	}	
	debug() << "\nSUM [std::string]: " << b << "\n";
	end = std::chrono::system_clock::now();
	elapsed_seconds = end-start;
	end_time = std::chrono::system_clock::to_time_t(end);
	std::cout << "newly made strings, elapsed time: " << elapsed_seconds.count() << "s\n";
	// -------------------------------------
	
	// -------------------------------------
	// lookup from existing MLSymbols
	// begin time
	start = std::chrono::system_clock::now();
	c = 0.f;
	idx = 0;
	for(int i=0; i<kTestLength; ++i)
	{
		if(++idx >= kTableSize) idx = 0;	
		c += testMap1[symbolDict[idx]];
	}
	debug() << "\nSUM [MLSymbol]: " << c << "\n";
	end = std::chrono::system_clock::now();
	elapsed_seconds = end-start;
	end_time = std::chrono::system_clock::to_time_t(end);
	std::cout << "existing symbols, elapsed time: " << elapsed_seconds.count() << "s\n";
	// -------------------------------------
	
	// -------------------------------------
	// lookup from new MLSymbols made from char * 
	// begin time
	start = std::chrono::system_clock::now();
	d = 0.f;
	idx = 0;
	for(int i=0; i<kTestLength; ++i)
	{
		if(++idx >= kTableSize) idx = 0;	
		d += testMap1[charDict[idx]];
	}
	debug() << "\nSUM [MLSymbol]: " << d << "\n";
	end = std::chrono::system_clock::now();
	elapsed_seconds = end-start;
	std::cout << "newly made symbols, elapsed time: " << elapsed_seconds.count() << "s\n";
	// -------------------------------------
	
	// assert(a == c);
	
	theSymbolTable().audit();

	// numbers test
	for(int i=0; i<10; ++i)
	{
		MLSymbol testSym = symbolDict[i];
		MLSymbol testSymWithNum = testSym.withFinalNumber(i);
		MLSymbol testSymWithoutNum = testSymWithNum.withoutFinalNumber();
		// assert testSym = testSymWithoutNum;
	}

	theSymbolTable().dump();
	theSymbolTable().audit();
	
	// multithreaded test. 
	theSymbolTable().clear();
	int nThreads = 10;
	std::vector< std::thread > threads;
	for(int i=0; i < nThreads; ++i)
	{
		threads.push_back(std::thread(threadTest, i));
	}
	for(int i=0; i < nThreads; ++i)
	{
		threads[i].join();
	}
	theSymbolTable().dump();
	theSymbolTable().audit();
	// assert(theSymbolTable.getSize() == 101);
	
	// TODO use assertions, compare results and make this test a thing that can pass or fail in a the context of a real test framework.
	
	return 0;
}




