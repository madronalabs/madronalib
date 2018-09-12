
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


TEST_CASE("madronalib/core/timer/basic", "[timer][basic]")
{

	int sum = 0;
	
	// print some numbers
	int n = 10;
	std::vector< std::unique_ptr< Timer > > v;
	for(int i=0; i<n; ++i)
	{
		v.emplace_back( std::unique_ptr< Timer > (new Timer));
		v[i]->callNTimes([&sum](){sum += 1; std::cout << ".";}, milliseconds(100 + 20*n), 2);
	}
	std::this_thread::sleep_for(milliseconds(1000));

	std::cout << "timer sum: " << sum << "\n";
	
	REQUIRE(sum == 20);
	
#ifdef _WINDOWS
	system("pause");
#endif

}
