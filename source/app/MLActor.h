// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLCollection.h"
#include "MLQueue.h"
#include "MLTimer.h"

// An Actor is a Collectable object that handles incoming messages
// using its own queue and timer. Combining Actors is a simple and scalable way
// to make distributed systems.

namespace ml
{

class Actor;
class ActorRegistry
{
  Tree< Actor* > _actors;
  std::mutex _listMutex;
  
public:
  ActorRegistry() = default;
  ~ActorRegistry() = default;
  
  Actor* getActor(Path actorName)
  {
    return _actors[actorName];
  }
  
  void doRegister(Path actorName, Actor* a)
  {
    _actors[actorName] = a;
  }
  
  void doRemove(Actor* actorToRemove)
  {
    // get exclusive access to the Tree
    std::unique_lock<std::mutex> lock(_listMutex);
    
    // remove the Actor
    for (auto it = _actors.begin(); it != _actors.end(); ++it)
    {
      Actor* pa = *it;
      if(pa == actorToRemove)
      {
        const Path p = it.getCurrentNodePath();
        _actors[p] = nullptr;
      }
    }
  }
};


// TODO actor can have state machine functions using PropertyTree as storage.

class Actor : public Collectable
{
  static constexpr size_t kMessageQueueSize{128};
  static constexpr size_t kDefaultMessageInterval{1000/60};
  
  Queue< Message > _messageQueue{kMessageQueueSize};
  Timer _queueTimer;
  
 protected:
  size_t _maxQueueSize{0};
  size_t _msgCounter{0};
  size_t getMessagesAvailable() { return _messageQueue.elementsAvailable(); }

  inline void handleMessageList(const MessageList& ml)
  {
    for (auto m : ml)
    {
      handleMessage(m);
    }
  }
  
  virtual void handleFullQueue(){}

 public:
  virtual void handleMessage(Message m) = 0;

  Actor() {}
  virtual ~Actor() = default;

  void start(size_t interval = kDefaultMessageInterval)
  {
    _queueTimer.start([=]() { handleMessagesInQueue(); }, milliseconds(interval));
  }

  void stop()
  {
    _queueTimer.stop();
  }

  // Collectable implementation
  inline void receiveMessage(Message m)
  {
    // queue returns true unless full.
    if(!(_messageQueue.push(m))) handleFullQueue();
    
    // DEBUG
    _maxQueueSize = std::max(_maxQueueSize, getMessagesAvailable());
  }

  inline void handleMessagesInQueue()
  {
    Message m;
    while (auto notEmpty = _messageQueue.pop(m))
    {
      handleMessage(m);
      _msgCounter++;
    }
  }
};

inline void registerActor(Path actorName, Actor* actorToRegister)
{
  SharedResourcePointer< ActorRegistry > registry;
  registry->doRegister(actorName, actorToRegister);
}

inline void removeActor(Actor* actorToRemove)
{
  SharedResourcePointer< ActorRegistry > registry;
  registry->doRemove(actorToRemove);
}

inline void sendMessage(Path actorName, Message m)
{
  SharedResourcePointer< ActorRegistry > registry;
  if(Actor* pActor = registry->getActor(actorName))
  {
    sendMessage(*pActor, m);
  }
}

}  // namespace ml
