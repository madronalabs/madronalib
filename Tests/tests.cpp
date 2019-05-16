//
//  tests.cpp
//  madronalib
//
//  Created by Randy Jones on 7/4/15.
//
//

#ifdef _WINDOWS

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "tests.h"

// Windows hack to pause console output so we can read it. 
#include <conio.h>
int main(int argc, char ** argv)
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

