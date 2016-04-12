
#include <iostream>
#include "../../include/madronalib.h"

using namespace ml;

int main()
{
	std::cout << "Hello, world!\n";
	
	DSPVector a;
	int size = kDSPVectorSizeFloat;
	
	for(int i=0; i<size; ++i)
	{
		a[i] = i;
	}

	// 	 x^y = exp(y * log(x))

	DSPVector diff = exp(a) ;

	std::cout << diff;
		
	return 0;
}
