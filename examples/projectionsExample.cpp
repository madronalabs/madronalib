//
//  projectionsExample.cpp
//  madronalib
//


#include <iostream>
#include <chrono>
#include <thread>

#include "mldsp.h"

using namespace ml;


int main()
{
	// map the interval [0, 10] to [0, 1000]
	Projection p1(projections::intervalMap({0, 10}, {10, 1000}));
	std::cout << "\n\n";
	for(int i=0; i<=10; ++i)
	{
		std::cout << i << " : " << p1(i) << "\n";
	}
	
	// log mapping of the interval [0, 10] to [0, 1000]
	Projection p(projections::intervalMap({0, 10}, {10, 1000}, projections::log({10, 1000})));
	std::cout << "\n\n";
	for(int i=0; i<=10; ++i)
	{
		std::cout << i << " : " << p(i) << "\n";
	}
	
	// flatcenter mapping of the interval [0, 10] to [0, 1000]
	Projection pp(projections::intervalMap({0, 10}, {10, 1000}, projections::flatcenter));
	std::cout << "\n\n";
	for(int i=0; i<=10; ++i)
	{
		std::cout << i << " : " << pp(i) << "\n";
	}
	
	// composition of two mappings: 
	// [0-10] to unity with a piecewise map
	// note that compose(a, b) returns f() = a(b())
	Projection q(compose(projections::piecewiseLinear({1, 97, 2}), projections::intervalMap({0, 10}, {0, 1})));
	std::cout << "\n\n";
	for(int i=0; i<=10; ++i)
	{
		std::cout << i << " : " << q(i) << "\n";
	}

#ifdef _WINDOWS
	system("pause");
#endif

	return 0;
}



