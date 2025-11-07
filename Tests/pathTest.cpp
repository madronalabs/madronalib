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

TEST_CASE("madronalib/core/path/symbolic", "[path]")
{
  Path p("hello/world/a/b/c/d/e/f/g");
  
  Path a{"a"};
  Path b{"b"};
  Path d{"d"};
  Path p4 (a, b, "c", d);
  Path p5 (p4, "george", p4);
  
  REQUIRE(p5.getSize() == 9);
  REQUIRE(p.beginsWith("hello/world"));
  REQUIRE(!p.beginsWith("hello/world/b"));
  REQUIRE(p.beginsWith(p));
  
  Path q(p, "and/more");
  REQUIRE(!p.beginsWith(q));
  
  auto f1 = butLast(p);
  auto f2 = lastN(p, 2);
  
  auto prop = Text("param");
  Path pname(prop);
  std::cout << "param: " << pname << "\n";
  
  // makes a path containing two unregistered Symbols.
  constexpr auto spaceThetaPath = HashedPath("space/theta");
  
  TODO a HashedPath is a specialization of GenericPath with hashes as keys
  REQUIRE(pathToText(spaceThetaPath) == "?/?");
  
  // making a non-constexpr Path registers the symbols.
  Path registersTheSymbols{"space/theta"};
  REQUIRE(pathToText(spaceThetaPath) == "space/theta");
}

TEST_CASE("madronalib/core/path/dynamic", "[path]")
{
  TextPath oneElementPath(TextFragment("foo").getText());
  
  TextPath p("hello/world/a/b/c/d/e/f/g");
  auto initialSize = theSymbolTable().getSize();
  
  auto accumTest = [](TextFragment a, TextFragment b) {
    return TextFragment(a, TextFragment("+"), b);
  };
  TextFragment accumTextResult = std::accumulate(++p.begin(), p.end(),
                                    *p.begin(), accumTest);
  REQUIRE(TextFragment("hello+world+a+b+c+d+e+f+g") == accumTextResult);

  TextPath a{"a"};
  TextPath b{"b"};
  TextPath d{"d"};
  TextPath p4 (a, b, "c", d);
  TextPath p5 (p4, "george", p4);
  
  REQUIRE(p5.getSize() == 9);
  REQUIRE(p.beginsWith("hello/world"));
  REQUIRE(!p.beginsWith("hello/world/b"));
  REQUIRE(p.beginsWith(p));
  
  TextPath q(p, "and/more");
  REQUIRE(!p.beginsWith(q));
  
  // DynamicPaths don't affect the Symbol Table
  REQUIRE(theSymbolTable().getSize() == initialSize);
}

TEST_CASE("madronalib/core/symbol/equality", "[symbol]")
{
  Symbol p("hello/world");
  REQUIRE(p == "hello/world");
  REQUIRE(p != "hello/worl");
}

TEST_CASE("madronalib/core/path/equality", "[path]")
{
  Path p("hello/world");
  REQUIRE(p == "hello/world");
  REQUIRE(p != "hello/worl");
}

TEST_CASE("madronalib/core/path/init-list", "[path]")
{
  Path sum;
  for(Path p : PathList{"menu/lfo/rate", "menu/lfo/ratio", "menu/lfo/amount", "menu/learn/amount"})
  {
    sum = Path(sum, p);
  }
  REQUIRE(sum.getSize() == 12);
}

