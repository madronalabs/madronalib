// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLProc.h"

using namespace ml;

// ----------------------------------------------------------------
#pragma mark factory

class ProcFactory
{
 private:
  // delete copy and move constructors and assign operators
  ProcFactory(ProcFactory const&) = delete;             // Copy construct
  ProcFactory(ProcFactory&&) = delete;                  // Move construct
  ProcFactory& operator=(ProcFactory const&) = delete;  // Copy assign
  ProcFactory& operator=(ProcFactory&&) = delete;       // Move assign

  typedef Proc* (*MLProcCreateFnT)(void);
  typedef std::map<ml::Symbol, MLProcCreateFnT> FnRegistryT;
  FnRegistryT procRegistry;

 public:
  ProcFactory() = default;
  ~ProcFactory() = default;

  // singleton: we only want one ProcFactory, even for multiple DSPEngines.
  static ProcFactory& theFactory()
  {
    static ProcFactory f;
    return f;
  }

  size_t registeredClasses() { return procRegistry.size(); }

  // register an object creation function by the name of the class.
  void registerFn(const ml::Symbol className, MLProcCreateFnT fn);

  // create a new object of the named class.
  Proc* create(const ml::Symbol className);  // MLTEST , MLDSPContext* context);

  // debug.
  void printRegistry(void);
};

// Subclasses of Proc make an MLProcRegistryEntry object.
// This class is passed a className and links a creation function
// for the subclass to the className in the registry.  This way the ProcFactory
// knows how to make them.
template <class T>
class ProcRegistryEntry
{
 public:
  // TODO add optional categories and other info
  ProcRegistryEntry(const char* className)
  {
    Symbol classSym(className);
    ProcFactory::theFactory().registerFn(classSym, createInstance);
    //		ProcInfo< T >::setClassName(classSym);
  }

  // return pointer to a new MLProc instance.
  static Proc* createInstance()
  {
    // TODO move semantics here?

    return new T;
  }

  int dummy{4};
};
