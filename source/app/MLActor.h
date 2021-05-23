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

  void stop() { _queueTimer.stop(); }

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

}  // namespace ml
