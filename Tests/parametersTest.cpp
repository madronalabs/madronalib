// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "madronalib.h"
#include "testUtils.h"

using namespace ml;

// Create some parameters.
// TODO read from JSON
void readParameterDescriptions(ParameterDescriptionList& params)
{
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "linear-param" },
    { "range", {0, 1} }
  } ) );

  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "log-param" },
    { "range", {0.001, 1} },
    { "log", true },
    { "plaindefault", 0.05 }
  } ) );
  
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "log-param-with-offset" },
    { "range", {1, 6} },
    { "log", true },
    { "offset", -1.f },
    { "plaindefault", 0.0 }
  } ) );
}

// Test to confirm that the projections are invertible
TEST_CASE("madronalib/core/parameters", "[parameters]")
{
  ParameterTree params;
  ParameterDescriptionList pdl;

  // make parameters and projections and set defaults
  readParameterDescriptions(pdl);
  
  // build the parameter tree, creating projections
  buildParameterTree(pdl, params);
  
  params.setValue("linear-param", 0.88f);
  auto v1 = params.getRealFloatValue("linear-param");
  
  std::cout << "value: " << v1 << "\n";
  
  params.paramsNorm_["foo"] = 12.f;
  
  
  // build name index
  std::vector< Path > paramNames;
  for(size_t i=0; i < pdl.size(); ++i)
  {
    ParameterDescription& pd = *pdl[i];
    paramNames.push_back(runtimePath(pd.getTextProperty("name")));
  }
  
  for(auto & pname : paramNames)
  {
    // TODO fix this weird access with a Parameter class (see MLParameters TODO)
    // this should allow us to forget about the name list (though that is sometimes useful)
    // and just write for(auto& param : params).
    ParameterDescription& pdesc = *params.descriptions[pname];
    ParameterProjection& pproj = params.projections[pname];
    
    int nSteps{10};
    for(int i=0; i<=nSteps; ++i)
    {
      float fNorm = i/float(nSteps);
      float fReal = pproj.normalizedToReal(fNorm);
      float fNorm2 = pproj.realToNormalized(fReal);
      
      REQUIRE(testUtils::nearlyEqual(fNorm, fNorm2));
    }
  }
}
