//
//  procsTest.cpp
//  madronalib
//

#include "catch.hpp"
#include "madronalib.h"
#include "MLProcFactory.h"

using namespace ml;

TEST_CASE("madronalib/core/procs", "[procs]")
{
	std::cout << "\nPROCS:\n";

  // factory gets a name, only for testing
  auto& factory(ProcFactory::theFactory());

  std::cout << factory.registeredClasses() << " proc classes registered.\n";
  
  // create will return nullptr if a ProcRegistryEntry for the proc name is not declared
	std::unique_ptr<Proc> pm (factory.create("multiply"));
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
