
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
		a[i] = i*kMLTwoPi/size - kMLPi;
	}

	// 	 x^y = exp(y * log(x))

	DSPVector diff = sin(a) ;

	DSPVector b(a);
	DSPVector c(diff);
	DSPVector ab = select(greaterThan(diff, DSPVector(0)), b, c);

	std::cout << ab;
		
	return 0;
}
