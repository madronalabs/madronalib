//
//  timerExample.cpp
//  madronalib
//


#include <iostream>
#include <chrono>
#include <thread>

#include "../include/madronalib.h"
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
	
	Timer u;
	
	// call from function
	u.start(functionB, milliseconds(200));

	{
		Timer t;
		
		// call from lambda
		t.start([](){ return functionA(); }, milliseconds(50));
		std::this_thread::sleep_for(milliseconds(1000));
	}
	
	u.stop();
	std::this_thread::sleep_for(milliseconds(1000));

	std::cout << "/main\n";

#ifdef _WINDOWS
	system("pause");
#endif

	return 0;
}



