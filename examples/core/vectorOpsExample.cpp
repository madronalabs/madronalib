
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
		a[i] = i;//*200./size;
	}

	// 	 x^y = exp(y * log(x))

	DSPVector diff = expApprox(logApprox(a)) ;
	
//	diff = select(greaterThan(diff, 5), 3, 2);
	

	std::cout << diff;
	
	return 0;
}
