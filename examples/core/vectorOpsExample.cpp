
#include <iostream>
#include "../../include/madronalib.h"

using namespace ml;

int main()
{
	std::cout << "Hello, world!\n";
	
	DSPVector a;
	int size = ml::kDSPVectorSizeFloat;
	
	for(int i=0; i<size; ++i)
	{
		a[i] = i;
	}
	
	DSPVector diff = exp(log(a));

	std::cout << diff;
		
	return 0;
}

// 