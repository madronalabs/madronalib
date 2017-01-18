
#include "MLProcMultiply.h"

// definition of static constexpr data member at namespace scope is still required, until C++17
constexpr str_const ProcMultiply::paramNames[];
constexpr str_const ProcMultiply::inputNames[];
constexpr str_const ProcMultiply::outputNames[];

void ProcMultiply::process() 
{
	output("baz") = multiply(input("foo"), input("bar"));
}

