// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLPath.h"
#include "MLValue.h"
#include "MLCollection.h"

namespace ml
{
struct Message final
{
  Path address{};
  Value value{};
  uint32_t flags{0};

  Message(Path h = Path(), Value v = Value(), uint32_t f = 0) : address(h), value(v), flags(f) {}
};

enum flags
{
  kMsgSequenceStart = 1 << 0,
  kMsgSequenceEnd = 1 << 1,
  kMsgFromController = 1 << 2,
  kMsgFromUI = 1 << 3
};

// note: because std::vector will allocate on the fly, this implementation of
// MessageList is not safe for use in audio processing threads. Given the
// intended use in editors and controllers, this seems like a reasonable
// tradeoff.

using MessageList = std::vector< Message >;

inline MessageList append(MessageList a, MessageList b)
{
  MessageList r{a};
  r.insert( r.end(), b.begin(), b.end() );
  return r;
}

struct MessageReceiver
{
  virtual void receiveMessage(Message m) = 0;
};

// send a message directly to a MessageReceiver.
inline void sendMessage(MessageReceiver& obj, Message m)
{
  obj.receiveMessage(m);
}

// send a message list directly to a MessageReceiver.
inline void sendMessages(MessageReceiver& obj, MessageList msgList)
{
  for(auto& m : msgList)
  {
    sendMessage(obj, m);
  }
}


// send a message to a MessageReceiver object through a Collection.
template <typename T>
inline void sendMessage(Collection<T> coll, Path p, Message m)
{
  if(auto& obj = coll.find(p))
  {
    obj->receiveMessage(m);
  }
}

// send a list of messages to a MessageReceiver object through a Collection.
template <typename T>
inline void sendMessages(Collection<T> coll, Path p, MessageList msgList)
{
  if (auto& obj = coll.find(p))
  {
    for(auto& m : msgList)
    {
      obj->receiveMessage(m);
    }
  }
}


// send a message to each direct child of the collection's root node.
template <typename T>
inline void sendMessageToEachChild(Collection<T> coll, Message m)
{
  coll.forEachChild([&](T& obj) { sendMessage(obj, m); });
}

// send a message to each direct child of the referenced collection's root node.
template <typename T>
inline void sendMessageToEachChild(typename Collection<T>::TreeType& collRef, Message m)
{
  Collection< T > subCollection(collRef);
  forEachChild< T >(subCollection, [&](T& obj) { sendMessage(obj, m); });
}

// send a message to each MessageReceiver in the collection.
template <typename T>
inline void sendMessageToEach(Collection<T> coll, Message m)
{
  forEach< T >(coll, [&](T& obj) { sendMessage(obj, m); });
}

// send a message to each MessageReceiver in the referenced collection.
template <typename T>
inline void sendMessageToEach(typename Collection<T>::TreeType& collRef, Message m)
{
  Collection< T > subCollection(collRef);
  forEach< T >(subCollection, [&](T& obj) { sendMessage(obj, m); });
}

}  // namespace ml

inline std::ostream& operator<<(std::ostream& out, const ml::Message& r)
{
  std::cout << "{" << r.address << ": " << r.value << "}";
  return out;
}
