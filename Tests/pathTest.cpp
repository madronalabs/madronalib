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
  TextFragment pTest("hello+world+a+b+c+d+e+f+g");
  
  auto accumTest = [](Symbol a, Symbol b) {
    return TextFragment(a.getTextFragment(), TextFragment("+"),
                        b.getTextFragment());
  };
  TextFragment accumTextResult = std::accumulate(++p.begin(), p.end(),
                                    (*p.begin()).getTextFragment(), accumTest);
  REQUIRE(TextFragment("hello+world+a+b+c+d+e+f+g") == accumTextResult);
  
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
  
  
  std::cout << "static: " << p << "\n";
}

TEST_CASE("madronalib/core/path/dynamic", "[path]")
{
  DynamicPath oneElementPath(TextFragment("foo"));
  
  DynamicPath p("hello/world/a/b/c/d/e/f/g");
  auto initialSize = theSymbolTable().getSize();
  
  auto accumTest = [](TextFragment a, TextFragment b) {
    return TextFragment(a, TextFragment("+"), b);
  };
  TextFragment accumTextResult = std::accumulate(++p.begin(), p.end(),
                                    *p.begin(), accumTest);
  REQUIRE(TextFragment("hello+world+a+b+c+d+e+f+g") == accumTextResult);

  DynamicPath a{"a"};
  DynamicPath b{"b"};
  DynamicPath d{"d"};
  DynamicPath p4 (a, b, "c", d);
  DynamicPath p5 (p4, "george", p4);
  
  REQUIRE(p5.getSize() == 9);
  REQUIRE(p.beginsWith("hello/world"));
  REQUIRE(!p.beginsWith("hello/world/b"));
  REQUIRE(p.beginsWith(p));
  
  DynamicPath q(p, "and/more");
  REQUIRE(!p.beginsWith(q));
  
  // DynamicPaths don't affect the Symbol Table
  REQUIRE(theSymbolTable().getSize() == initialSize);
  
  std::cout << "dynamic: " << p << "\n";
}


