
//
// symbolTest
// a unit test made using the Catch framework in catch.hpp / tests.cpp.
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

#if _WIN32
#define HAVE_U8_LITERALS 0
#else
#define HAVE_U8_LITERALS 1
#endif

const char letters[24] = "abcdefghjklmnopqrstuvw";

TEST_CASE("madronalib/core/symbol/maps", "[symbol]")
{
	const int kTableSize = 100;	
	const int kTestLength = 100000;
	
	// main maps for testing
	std::map<MLSymbol, float> testMapOrderedSym;
	std::map<std::string, float> testMapOrderedStr;
	std::unordered_map<MLSymbol, float> testMapUnorderedSym;
	std::unordered_map<std::string, float> testMapUnorderedStr;
	
	// make dictionaries of symbols, strings and chars for testing
	MLNameMaker nameMaker;
	std::vector<MLSymbol> symbolDict;
	std::vector<std::string> stringDict;
	std::vector<const char *> charDict;
	int p = 0;
	for(int i=0; i<kTableSize; ++i)
	{
		// make procedural gibberish
		std::string newString;
		int length = 3 + (p%8);
		for(int j=0; j<length; ++j)
		{
			p += (i*j + 1);
			p += i%37;
			p += j%23;
			p = abs(p);
			newString += letters[p % 22];
		}		
		
		stringDict.push_back(newString);
		
		// add it to symbol table
		MLSymbol newSym(newString.c_str());
		symbolDict.push_back(newSym);
		
		// add an entry to each map
		float val = i;
		testMapOrderedSym[newSym] = val;
		testMapOrderedStr[newString] = val;
		testMapUnorderedSym[newSym] = val;
		testMapUnorderedStr[newString] = val;
	}
	
	// make char dict after string dict is complete, otherwise ptrs may change!
	for(int i=0; i<stringDict.size(); ++i)
	{
		charDict.push_back(stringDict[i].c_str());
	}
	
	SECTION("test maps")
	{
		std::chrono::time_point<std::chrono::system_clock> start, end;
		std::chrono::duration<double> elapsed;
		double symbolSum, stringSum;
		int idx;
		
		// lookup from existing std::strings
		start = std::chrono::system_clock::now();
		stringSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			stringSum += testMapOrderedStr[stringDict[idx]];
		}	
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "existing strings, elapsed time: " << elapsed.count() << "s\n";
		
		// lookup from existing MLSymbols
		start = std::chrono::system_clock::now();
		symbolSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			symbolSum += testMapOrderedSym[symbolDict[idx]];
		}
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "existing symbols, elapsed time: " << elapsed.count() << "s\n";
		
		REQUIRE(stringSum == symbolSum);
		
		// lookup from existing std::strings
		start = std::chrono::system_clock::now();
		stringSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			stringSum += testMapUnorderedStr[stringDict[idx]];
		}	
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "existing strings, unordered, elapsed time: " << elapsed.count() << "s\n";
		
		// lookup from existing MLSymbols
		start = std::chrono::system_clock::now();
		symbolSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			symbolSum += testMapUnorderedSym[symbolDict[idx]];
		}
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "existing symbols, unordered, elapsed time: " << elapsed.count() << "s\n";

		REQUIRE(stringSum == symbolSum);
		
		// lookup from newly made std::strings
		start = std::chrono::system_clock::now();
		stringSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			stringSum += testMapOrderedStr[charDict[idx]];
		}	
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "constructing strings, elapsed time: " << elapsed.count() << "s\n";
		
		// lookup from new MLSymbols made from char * 
		start = std::chrono::system_clock::now();
		symbolSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			symbolSum += testMapOrderedSym[charDict[idx]];
		}
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "constructing symbols, elapsed time: " << elapsed.count() << "s\n";

		REQUIRE(stringSum == symbolSum);
				
		// lookup from newly made std::strings
		start = std::chrono::system_clock::now();
		stringSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			stringSum += testMapUnorderedStr[charDict[idx]];
		}	
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "constructing strings, unordered, elapsed time: " << elapsed.count() << "s\n";
		
		// unordered lookup from new MLSymbols made from char * 
		start = std::chrono::system_clock::now();
		symbolSum = 0.f;
		idx = 0;
		for(int i=0; i<kTestLength; ++i)
		{
			if(++idx >= kTableSize) idx = 0;	
			symbolSum += testMapUnorderedSym[charDict[idx]];
		}
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "constructing symbols, unordered, elapsed time: " << elapsed.count() << "s\n";
				
		REQUIRE(stringSum == symbolSum);
		
		REQUIRE(theSymbolTable().audit());
	}
}

TEST_CASE("madronalib/core/symbol/numbers", "[symbol]")
{
	MLNameMaker namer;
	for(int i=0; i<10; ++i)
	{
		MLSymbol testSym = namer.nextNameAsString().c_str();
		MLSymbol testSymWithNum = testSym.withFinalNumber(i);
		MLSymbol testSymWithoutNum = testSymWithNum.withoutFinalNumber();
		REQUIRE(testSym == testSymWithoutNum);
	}
	REQUIRE(theSymbolTable().audit());
}

static const int kThreadTestSize = 1024;

void threadTest(int threadID)
{
	MLNameMaker namer;
	for(int i=0; i<kThreadTestSize; ++i)
	{
		MLSymbol sym(namer.nextNameAsString().c_str());
		std::this_thread::yield();
	}
}

TEST_CASE("madronalib/core/symbol/threads", "[symbol][threads]")
{
	// multithreaded test. multiple nameMakers will try to make duplicate names at about the same time,
	// which will almost certainly lead to problems unless the symbol library is properly thread-safe.
	
	theSymbolTable().clear();
	int nThreads = 16;
	std::vector< std::thread > threads;
	for(int i=0; i < nThreads; ++i)
	{
		threads.push_back(std::thread(threadTest, i));
	}
	for(int i=0; i < nThreads; ++i)
	{
		threads[i].join();
	}
	
	REQUIRE(theSymbolTable().audit());
	REQUIRE(theSymbolTable().getSize() == kThreadTestSize + 1);
}


TEST_CASE("madronalib/core/symbol/identity", "[symbol][identity]")
{
	// things that should and shouldn't be the same as one another.
	theSymbolTable().clear();
	MLSymbol a("xxx_yyy");
	MLSymbol b("xxx");
	REQUIRE(a != b);
}

#if USE_ALPHA_SORT

TEST_CASE("madronalib/core/symbol/sorting", "[symbol][sorting]")
{
	theSymbolTable().clear();
	std::map<MLSymbol, int> sortedMap;
	const int sortTestSize = 10;
	int p = 0;

	// make procedural gibberish as keys for sorted map.
	for(int i=0; i<sortTestSize; ++i)
	{
		std::string newString;
		int length = 3 + (p%8);
		for(int j=0; j<length; ++j)
		{
			p += (i*j + 1);
			p += i%37;
			p += j%23;
			p = abs(p);
			newString += letters[p % 22];
		}		
		sortedMap[newString] = i;
	}
	
	// make sure each successive pair of keys is in sorted order.
	MLSymbol symA, symB;
	for(auto mapEntry : sortedMap)
	{		
		symA = symB;
		symB = mapEntry.first;
		REQUIRE(symA < symB);
	}
}

#endif


// hex char printing
struct HexCharStruct
{
	unsigned char c;
	HexCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs)
{
	return (o << std::hex << (int)hs.c << std::dec );
}

inline HexCharStruct hexchar(unsigned char _c)
{
	return HexCharStruct(_c);
}

TEST_CASE("madronalib/core/symbol/UTF8", "[symbol][UTF8]")
{
	theSymbolTable().clear();
	std::map<MLSymbol, int> sortedMap;
	const int sortTestSize = 10;
	int p = 0;

#if HAVE_U8_LITERALS
	std::vector< std::string > strings = { u8"Федор", u8"小林 尊", u8"محمد بن سعيد" };
#else
	const char* fedor("\xD0\xA4\xD0\xB5\xD0\xB4\xD0\xBE\xD1\x80");
	const char* kobayashi("\xE5\xB0\x8F\xE6\x9E\x97\x20\xE5\xB0\x8A");
	const char* muhammad("\xD9\x85\xD8\xAD\xD9\x85\xD8\xAF\x20\xD8\xA8\xD9\x86\x20\xD8\xB3\xD8\xB9\xD9\x8A\xD8\xAF");
	std::vector< std::string > strings = { fedor, kobayashi, muhammad };
#endif	
	
	for(auto testString : strings)
	{
		MLSymbol testSym(testString.c_str());
		std::string strB = testSym.getString();
		int lenB = strB.length();
		std::cout << strB << " : ";
		for(int i=0; i<lenB; ++i)
		{
			std::cout << hexchar(strB[i]) << " ";
		}
		std::cout << "\n";
		// TODO write some actual test here
	}
}

// TEMP to move to new test file about string utils / paths
TEST_CASE("madronalib/core/symbol/paths", "[symbol][paths]")
{
	theSymbolTable().clear();
	
	std::vector< std::string > path = ml::stringUtils::parsePath(std::string("this/is/a/path/to/a/小林 尊/interesting/Федор/this/has/some spaces in it"));
	
	std::cout << "as elements: ";
	for(auto elem : path)
	{
		std::cout << elem << "/";
	}
	std::cout << "\n";
	//theSymbolTable().dump();
}



