// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLActor.h"

using namespace ml;

Actor* ActorRegistry::getActor(Path actorName) { return _actors[actorName]; }

void ActorRegistry::doRegister(Path actorName, Actor* a) { _actors[actorName] = a; }

void ActorRegistry::doRemove(Actor* actorToRemove)
{
  // get exclusive access to the Tree
  std::unique_lock<std::mutex> lock(_listMutex);

  // remove the Actor
  for (auto it = _actors.begin(); it != _actors.end(); ++it)
  {
    Actor* pa = *it;
    if (pa == actorToRemove)
    {
      auto p = it.getCurrentPath();
      _actors[p] = nullptr;
    }
  }
}

void ActorRegistry::dump() { _actors.dump(); }
