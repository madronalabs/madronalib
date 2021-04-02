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
  static constexpr size_t kMessageQueueSize = 16;
  Queue< Message > _messageQueue{ kMessageQueueSize };
  Timer _queueTimer;
  
protected:
  size_t getMessagesAvailable() { return _messageQueue.elementsAvailable(); }
  
public:
  // We can think of an Actor, mainly, as a message handler.
  virtual void handleMessage(Message m) = 0;
  
  Actor(){}
  virtual ~Actor() = default;
  
  void start()
  {
    _queueTimer.start([=](){ handleMessages(); }, milliseconds(1000/60));
  }
  
  void stop()
  {
    _queueTimer.stop();
  }

  // Collectable
  inline void recv(Message m)
  {
    // TEMP todo error check
    auto ok = _messageQueue.push(m);
  }
  
  inline void handleMessages()
  {
    Message m;
    while(auto notEmpty = _messageQueue.pop(m))
    {
      handleMessage(m);
    }
  }
};


} // namespace ml
