//
//  procsTest.cpp
//  madronalib
//

#include "MLProcFactory.h"
#include "catch.hpp"
#include "madronalib.h"

using namespace ml;

TEST_CASE("madronalib/core/procs", "[procs]")
{
  std::cout << "\nPROCS:\n";

  // factory gets a name, only for testing
  auto& factory(ProcFactory::theFactory());

  std::cout << factory.registeredClasses() << " proc classes registered.\n";

  // real work in progress / sketch stuff. should a DSP context be required to
  // create a Proc? Probably.

  // create will return nullptr if a ProcRegistryEntry for the proc name is not
  // declared
  std::unique_ptr<Proc> pm(factory.create("multiply"));
  REQUIRE(pm != nullptr);

  pm->setParam("a", 3.14f);

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
