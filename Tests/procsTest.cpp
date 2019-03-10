//
//  procsTest.cpp
//  madronalib
//

#include "catch.hpp"
#include "madronalib.h"

using namespace ml;
TEST_CASE("madronalib/core/procs", "[procs]")
{
	std::cout << "\nPROCS:\n";

	// create will return nullptr if a ProcRegistryEntry for the proc name is not declared
	std::unique_ptr<Proc> pm (ProcFactory::theFactory().create("multiply")); 
	REQUIRE(pm != nullptr);
	
	DSPVector va, vb, vc;
	va = 2;
	vb = 3;
	
	pm->setInput("foo", va);
	pm->setInput("bar", vb);
	pm->setOutput("baz", vc);
	pm->process();
	
	REQUIRE(vc == multiply(va, vb));
	
	std::cout << "output: " << vc << "\n";	
}

