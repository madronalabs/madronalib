// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/


#pragma once

#include "MLCollection.h"
#include "MLParameter.h"

namespace ml{

// actor can have state machine functions using PropertyTree as storage.

class Actor : public Collectable
{
  static constexpr size_t kMessageQueueSize = 128;
  Queue< Message > _messageQueue{ kMessageQueueSize };
  Timer _queueTimer;
  
protected:
  size_t getMessagesAvailable() { return _messageQueue.elementsAvailable(); }
  
  inline void handleMessageList(const MessageList& ml)
  {
    for(auto m : ml)
    {
      handleMessage(m);
    }
  }

public:
  // We can think of an Actor, mainly, as a message handler.
  virtual void handleMessage(Message m) = 0;
  
  Actor(){}
  virtual ~Actor() = default;
  
  void start()
  {
    _queueTimer.start([=](){ handleMessagesInQueue(); }, milliseconds(1000/60));
  }
  
  void stop()
  {
    _queueTimer.stop();
  }

  // Collectable
  inline Value respond(Message m)
  {
    // return true, unless queue was full.
    return(_messageQueue.push(m));
  }
  
  inline void handleMessagesInQueue()
  {
    Message m;
    while(auto notEmpty = _messageQueue.pop(m))
    {
      handleMessage(m);
    }
  }
};


} // namespace ml
