
//
// timerTest
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
#include "MLTimer.h"

using namespace ml;
using namespace std::chrono;

/*
typedef time_point<high_resolution_clock> myTimePoint;
myTimePoint now()
{
	return high_resolution_clock::now();
}
*/

int functionA(void)
{
	std::cout << "TICK\n";
	return true;
}


TEST_CASE("madronalib/core/timer/basic", "[timer][basic]")
{
	
	
	
}
