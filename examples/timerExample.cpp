//
//  timerExample.cpp
//  madronalib
//


#include <iostream>
#include <chrono>
#include <thread>

#include "madronalib.h"
#include "MLTimer.h"

using namespace ml;

int functionA(void)
{
	std::cout << "TICK\n";
	return true;
}

int functionB(void)
{
	std::cout << "TOCK\n";
	return true;
}

int main()
{
	std::cout << "main\n";

  Timers::theTimers().start();
	
	Timer u;
	
	// call from function
	u.start(functionB, milliseconds(200));

	{
		// call from lambda
		Timer t;
		t.start([](){ return functionA(); }, milliseconds(50));
		std::this_thread::sleep_for(milliseconds(1000));
	}
	
	u.stop();
	std::this_thread::sleep_for(milliseconds(100));
	
	// print some newlines
	Timer newlines;
	newlines.start([=](){std::cout << "\n";}, milliseconds(100));
	
	// print some numbers
	int n = 10;
	std::vector< std::unique_ptr< Timer > > v;
	for(int i=0; i<n; ++i)
	{
		v.emplace_back( std::unique_ptr< Timer > (new Timer));
		v[i]->start([=](){std::cout << i << " ";}, milliseconds(100 + 10*n));
		std::this_thread::sleep_for(milliseconds(100));
	}
	std::this_thread::sleep_for(milliseconds(1000));

	std::cout << "/main\n";

#ifdef _WINDOWS
	system("pause");
#endif

	return 0;
}



