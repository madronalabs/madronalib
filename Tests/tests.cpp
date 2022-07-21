// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#ifdef _WINDOWS

#define CATCH_CONFIG_RUNNER
#include "tests.h"

#include "catch.hpp"

// Windows hack to pause console output so we can read it.
#include <conio.h>
int main(int argc, char** argv)
{
  int result = Catch::Session().run(argc, argv);
  system("pause");
  return result;
}

#else

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "tests.h"

#endif
