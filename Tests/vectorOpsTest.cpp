
//
// vectorOpsTest
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
#include "MLVectorOps.h"

using namespace ml;


TEST_CASE("madronalib/core/vector_ops", "[vector_ops]")
{
	SECTION("time")
	{
		int trials = 1000000;
		std::chrono::time_point<std::chrono::system_clock> start, end;
		std::chrono::duration<double> elapsed;
		double symbolSum, stringSum;
		int idx;

		DSPVector a;
		DSPVector sum(0);
		int size = kDSPVectorSizeFloat;			
		for(int i=0; i<size; ++i)
		{
			a[i] = i*kMLTwoPi/size - kMLPi;
		}
		
		// do a thing
		start = std::chrono::system_clock::now();
		
		for(int i=0; i<trials; ++i)
		{
			DSPVector diff = cosApprox(a);	
			sum += diff;
		}
		
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "static constants, elapsed time: " << elapsed.count() << "s\n";
		std::cout << "sum: " << sum << "\n";
		
		sum = 0;
		
		// do a thing
		start = std::chrono::system_clock::now();

		
		end = std::chrono::system_clock::now();
		elapsed = end-start;
		std::cout << "set_ps1, elapsed time: " << elapsed.count() << "s\n";
		std::cout << "sum: " << sum << "\n";
		
	}
}
