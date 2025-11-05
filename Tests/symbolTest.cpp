// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "madronalib.h"
#include "testUtils.h"

#if _WIN32
#define HAVE_U8_LITERALS 0
#else
#define HAVE_U8_LITERALS 1
#endif

using namespace ml;

TEST_CASE("madronalib/core/symbol/hash", "[symbol]")
{
  // the compile time and runtime hashes need to be equivalent.
  
  // constexpr
  auto a1 = hash("hello");
  auto a2 = hash(u8"محمد بن سعيد");
  
  // runtime
  const char* str1("hello");
  const char* str2(u8"محمد بن سعيد");
  auto b1 = fnv1aRuntime(str1);
  auto b2 = fnv1aRuntime(str2);
  
  REQUIRE(a1 == b1);
  REQUIRE(a2 == b2);
}

TEST_CASE("madronalib/core/symbol/simple", "[symbol][simple]")
{
  Symbol a("hello");
  Symbol b("world");
  Symbol c("hello");

  REQUIRE(a.getHash() == c.getHash());
  REQUIRE(a.getHash() != b.getHash());
  REQUIRE(a == c);
}

typedef std::chrono::time_point<std::chrono::high_resolution_clock> myTimePoint;
myTimePoint now() { return std::chrono::high_resolution_clock::now(); }

void threadTest(TextFragment prefix, int n)
{
  textUtils::NameMaker namer;
  for (int i = 0; i < n; ++i)
  {
    TextFragment t(TextFragment(prefix), namer.nextName());
    Symbol sym = runtimeSymbol(t);
    std::this_thread::yield();
  }
}

TEST_CASE("madronalib/core/symbol/threads", "[symbol][threads]")
{
  // multithreaded test. multiple threadTests will try to make duplicate names at
  // about the same time, which will almost certainly lead to problems unless
  // the symbol code is properly thread-safe.

  int nThreads = 16;
  int nSymbolsPerThread = 100;

  // no duplicate symbols
  theSymbolTable().clear();
  std::vector<std::thread> threads;
  for (int i = 0; i < nThreads; ++i)
  {
    threads.push_back(std::thread(threadTest, TextFragment('A' + i), 100));
  }
  for (int i = 0; i < nThreads; ++i)
  {
    threads[i].join();
  }
  REQUIRE(theSymbolTable().getSize() == nThreads*nSymbolsPerThread);
  
  // all duplicate symbols
  theSymbolTable().clear();
  threads.clear();
  for (int i = 0; i < nThreads; ++i)
  {
    threads.push_back(std::thread(threadTest, TextFragment('A'), 100));
  }
  for (int i = 0; i < nThreads; ++i)
  {
    threads[i].join();
  }
  REQUIRE(theSymbolTable().getSize() == nSymbolsPerThread);
}


const char letters[24] = "abcdefghjklmnopqrstuvw";

TEST_CASE("madronalib/core/symbol/maps", "[symbol]")
{
  const int kMapSize = 100;
  const int kTestLength = 100000;

  // main maps for testing
  std::map<Symbol, float> testMapOrderedSym;
  std::map<std::string, float> testMapOrderedStr;
  std::unordered_map<Symbol, float> testMapUnorderedSym;
  std::unordered_map<std::string, float> testMapUnorderedStr;

  // make dictionaries of symbols, strings and chars for testing
  std::vector<Symbol> symbolDict;
  std::vector<std::string> stringDict;
  std::vector<const char*> charDict;
  int p = 0;
  for (int i = 0; i < kMapSize; ++i)
  {
    // make procedural gibberish
    std::string newString;
    int length = 3 + (i % 12);
    for (int j = 0; j < length; ++j)
    {
      p += (i * j + 1);
      p += i % 37;
      p += j % 23;
      p = abs(p);
      newString += letters[p % 22];
    }

    stringDict.push_back(newString);

    // add it to symbol table
    Symbol newSym = runtimeSymbol(newString.c_str());
    symbolDict.push_back(newSym);

    // add an entry to each map
    float val = i;
    testMapOrderedSym[newSym] = val;
    testMapOrderedStr[newString] = val;
    testMapUnorderedSym[newSym] = val;
    testMapUnorderedStr[newString] = val;
  }

  // make char dict after string dict is complete, otherwise ptrs may change!
  for (int i = 0; i < stringDict.size(); ++i)
  {
    charDict.push_back(stringDict[i].c_str());
  }

  SECTION("test maps")
  {
    myTimePoint start, end;
    std::chrono::duration<double> elapsed;
    double symbolSum, stringSum;
    int idx;
    RandomScalarSource randSource;

    std::vector<int> testIndexes;
    for (int n = 0; n < kTestLength; ++n)
    {
      int i = fabs(randSource.getFloat()) * kMapSize;
      testIndexes.push_back(i);
    }

    // ----------------------------------------------------------------
    // existing symbols / strings

    // lookup from existing std::strings
    start = now();
    stringSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      stringSum += testMapOrderedStr[stringDict[testIndexes[i]]];
    }
    end = now();
    elapsed = end - start;
    std::cout << "existing strings, elapsed time: " << elapsed.count() << "s\n";

    // lookup from existing Symbols
    start = now();
    symbolSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      symbolSum += testMapOrderedSym[symbolDict[testIndexes[i]]];
    }
    end = now();
    elapsed = end - start;
    std::cout << "existing Symbols, elapsed time: " << elapsed.count() << "s\n";

    REQUIRE(stringSum == symbolSum);

    // lookup from existing std::strings
    start = now();
    stringSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      stringSum += testMapUnorderedStr[stringDict[testIndexes[i]]];
    }
    end = now();
    elapsed = end - start;
    std::cout << "existing strings, unordered, elapsed time: " << elapsed.count() << "s\n";

    // lookup from existing Symbols
    start = now();
    symbolSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      symbolSum += testMapUnorderedSym[symbolDict[testIndexes[i]]];
    }
    end = now();
    elapsed = end - start;
    std::cout << "existing Symbols, unordered, elapsed time: " << elapsed.count() << "s\n";

    REQUIRE(stringSum == symbolSum);

    // ----------------------------------------------------------------
    // constructing symbols / strings

    // lookup from newly made std::strings
    start = now();
    stringSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      stringSum += testMapOrderedStr[charDict[testIndexes[i]]];
    }
    end = now();
    elapsed = end - start;
    std::cout << "new strings, elapsed time: " << elapsed.count() << "s\n";

    // lookup from new Symbols made from char *
    start = now();
    symbolSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      symbolSum += testMapOrderedSym[runtimeSymbol(charDict[testIndexes[i]])];
    }
    end = now();
    elapsed = end - start;
    std::cout << "new Symbols, elapsed time: " << elapsed.count() << "s\n";

    REQUIRE(stringSum == symbolSum);

    // lookup from newly made std::strings
    start = now();
    stringSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      stringSum += testMapUnorderedStr[charDict[testIndexes[i]]];
    }
    end = now();
    elapsed = end - start;
    std::cout << "new strings, unordered, elapsed time: " << elapsed.count() << "s\n";

    // unordered lookup from new Symbols made from char *
    start = now();
    symbolSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      symbolSum += testMapUnorderedSym[runtimeSymbol(charDict[testIndexes[i]])];
    }
    end = now();
    elapsed = end - start;
    std::cout << "new Symbols, unordered, elapsed time: " << elapsed.count() << "s\n";

    REQUIRE(stringSum == symbolSum);

    // lookup using literal (hashed at compile time)
    // examples generated by above: fkjcouvrhqtrk uhe cbtktb wfdq
    start = now();
    symbolSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      symbolSum += testMapOrderedSym["fkjcouvrhqtrk"];
    }
    end = now();
    elapsed = end - start;
    std::cout << "literals, elapsed time: " << elapsed.count() << "s\n";

    // unordered lookup using literal (hashed at compile time)
    start = now();
    symbolSum = 0.f;
    for (int i = 0; i < kTestLength; ++i)
    {
      symbolSum += testMapUnorderedSym["fkjcouvrhqtrk"];
    }
    end = now();
    elapsed = end - start;
    std::cout << "literals, unordered, elapsed time: " << elapsed.count() << "s\n";


    // theSymbolTable().dump();
  }
}

TEST_CASE("madronalib/core/symbol/numbers", "[symbol]")
{
  textUtils::NameMaker namer;
  for (int i = 0; i < 10; ++i)
  {
    int num = i*39620;
    Symbol testSym = runtimeSymbol(namer.nextName());
    
    Symbol testSymWithNum = textUtils::addFinalNumber(testSym, num);
    Symbol testSymWithoutNum = textUtils::stripFinalNumber(testSym);
    int finalNumber = textUtils::getFinalNumber(testSymWithNum);

    REQUIRE(testSym == testSymWithoutNum);
    REQUIRE(num == finalNumber);
  }
}

TEST_CASE("madronalib/core/symbol/identity", "[symbol][identity]")
{
  // things that should and shouldn't be the same as one another.
  //theSymbolTable().clear();
  Symbol a("xxx_yyy");
  Symbol b("xxx");
  REQUIRE(a != b);
}


// hex char printing
struct HexCharStruct
{
  unsigned char c;
  HexCharStruct(unsigned char _c) : c(_c) {}
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs)
{
  return (o << std::hex << (int)hs.c << std::dec);
}

inline HexCharStruct hexchar(unsigned char _c) { return HexCharStruct(_c); }

TEST_CASE("madronalib/core/symbol/UTF8", "[symbol][UTF8]")
{
  std::map<Symbol, int> sortedMap;
  const int sortTestSize = 10;
  int p = 0;

#if HAVE_U8_LITERALS
  std::vector<std::string> strings = {u8"Федор", u8"小林 尊", u8"محمد بن سعيد"};
#else
  const char* fedor("\xD0\xA4\xD0\xB5\xD0\xB4\xD0\xBE\xD1\x80");
  const char* kobayashi("\xE5\xB0\x8F\xE6\x9E\x97\x20\xE5\xB0\x8A");
  const char* muhammad(
      "\xD9\x85\xD8\xAD\xD9\x85\xD8\xAF\x20\xD8\xA8\xD9\x86\x20\xD8\xB3\xD8\xB9"
      "\xD9\x8A\xD8\xAF");
  std::vector<std::string> strings = {fedor, kobayashi, muhammad};
#endif

  int totalPoints = 0;
  for (auto testString : strings)
  {
    totalPoints += TextFragment(testString.c_str()).lengthInCodePoints();
  }

  REQUIRE(totalPoints == 21);
}
