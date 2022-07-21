// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLProcFactory.h"

// ----------------------------------------------------------------
#pragma mark factory

void ProcFactory::registerFn(const ml::Symbol className, MLProcCreateFnT fn)
{
  procRegistry[className] = fn;
}

Proc* ProcFactory::create(const ml::Symbol className)  // MLTEST, MLDSPContext* context)
{
  MLProcCreateFnT fn;
  Proc* resultProc = nullptr;

  // get named entry from registry
  FnRegistryT::const_iterator regEntry = procRegistry.find(className);

  if (regEntry != procRegistry.end())
  {
    // get creator fn from entry
    fn = regEntry->second;
    // call creator fn returning new MLProc subclass instance
    resultProc = fn();

    // MLTEST
    // resultProc->setContext(context);
  }

  return resultProc;
}

void ProcFactory::printRegistry(void)
{
  // int size = (int)procRegistry.size();

  /*
  debug() << "---------------------------------------\n";
  debug() << "MLProc registry: " << size << " members\n";

  for (FnRegistryT::iterator i = procRegistry.begin(); i != procRegistry.end();
  i++)
  {
          debug() << (*i).first << "\n";
  }
   */
}
