// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLMessage.h"
#include "MLQueue.h"
#include "MLTimer.h"

// An Actor handles incoming messages using its own queue and timer.
// Combining Actors is a simple and scalable way to make distributed systems.
// This is a very minimal implementation of the concept, with only the features
// needed for applications in current development.

namespace ml
{

class Actor;
class ActorRegistry
{
  Tree<Actor*> _actors;
  std::mutex _listMutex;

 public:
  ActorRegistry() = default;
  ~ActorRegistry() = default;

  Actor* getActor(Path actorName);
  void doRegister(Path actorName, Actor* a);
  void doRemove(Actor* actorToRemove);

  void dump();
};

class Actor
{
  friend ActorRegistry;

  static constexpr size_t kMessageQueueSize{128};
  static constexpr size_t kDefaultMessageInterval{1000 / 60};

  Queue<Message> _messageQueue{kMessageQueueSize};
  Timer _queueTimer;

 protected:
  size_t getMessagesAvailable() { return _messageQueue.elementsAvailable(); }

  // handle all the messages in the queue immediately.
  void handleMessagesInQueue()
  {
    while (Message m = _messageQueue.pop())
    {
      onMessage(m);
    }
  }

 public:
  Actor() = default;
  virtual ~Actor() = default;

  // Actors can override onFullQueue to specify what action to take when
  // the message queue is full.
  virtual void onFullQueue() {}

  // To make it clear that Actor is not a subclass of MessageReceiver, the virtual
  // handler method has a different name.
  virtual void onMessage(Message m) = 0;

  // delete copy and move constructors and assign operators
  Actor(Actor const&) = delete;             // Copy construct
  Actor(Actor&&) = delete;                  // Move construct
  Actor& operator=(Actor const&) = delete;  // Copy assign
  Actor& operator=(Actor&&) = delete;       // Move assign

  void start(size_t interval = kDefaultMessageInterval)
  {
    // we currently attempt to handle all the messages in the queue.
    // in the future we may want to do just a few at a time instead.
    _queueTimer.start([=]() { handleMessagesInQueue(); }, milliseconds(interval));
  }

  void stop() { _queueTimer.stop(); }

  // enqueueMessage just pushes the message onto the queue.
  void enqueueMessage(Message m)
  {
    // queue returns true unless full.
    if (!(_messageQueue.push(m)))
    {
      onFullQueue();
    }
  }

  void enqueueMessageList(const MessageList& ml)
  {
    for (auto m : ml)
    {
      enqueueMessage(m);
    }
  }
};

inline void registerActor(Path actorName, Actor* actorToRegister)
{
  SharedResourcePointer<ActorRegistry> registry;
  registry->doRegister(actorName, actorToRegister);
}

inline void removeActor(Actor* actorToRemove)
{
  SharedResourcePointer<ActorRegistry> registry;
  registry->doRemove(actorToRemove);
}

// send message to an Actor.
// if the named Actor exists, its onMessage method will be called.
//
// TODO handle situations where getActor() can't return a pointer
// to the receiver, as when it is in a different process or on a different computer.
// to start we will need a "main" host with the registry.
// - if receiver is in same process?
//    then send directly as below.
// -- else
//      serialize the message
//      transmit serialized message to the receiver's process or host (UDP)
inline void sendMessageToActor(Path actorName, Message m)
{
  SharedResourcePointer<ActorRegistry> registry;
  if (Actor* pActor = registry->getActor(actorName))
  {
    pActor->enqueueMessage(m);
  }
}

}  // namespace ml
