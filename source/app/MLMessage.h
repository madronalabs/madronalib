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
  friend std::ostream& operator<<(std::ostream& out, const Message& r);

  Path address{};
  Value value{};
  uint32_t flags{0};

  Message(Path h = Path(), Value v = Value(), uint32_t f = 0) : address(h), value(v), flags(f) {}
  
  explicit operator bool() const { return(address != Path()); }
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

struct MessageList : public std::vector< Message >
{
  void append(const MessageList& b)
  {
    insert(end(), b.begin(), b.end());
  }
};

struct MessageReceiver
{
protected:
  virtual void handleMessage(Message m, Message* replyPtr) = 0;
  
  MessageList processMessageList(MessageList msgList)
  {
    MessageList retList;
    Message resultMsg;
    
    for(auto msg : msgList)
    {
      resultMsg = Message();
      handleMessage(msg, &resultMsg);
      if(resultMsg)
      {
        retList.push_back(resultMsg);
      }
    }
    return retList;
  }
  
public:
  void doSendMessage(Message m)
  {
    handleMessage(m, nullptr);
  }
  
  void doSendRequest(Message m, Message* replyPtr)
  {
    handleMessage(m, replyPtr);
  }
};

// sending messages to MessageReceivers

// send a message directly to a MessageReceiver.
// If the sender wants a reply, it can specify a replyPtr.
inline void sendMessage(MessageReceiver& obj, Message m)
{
  obj.doSendMessage(m);
}

// send a message directly to a MessageReceiver.
// If the sender wants a reply, it can specify a replyPtr.
inline void sendRequest(MessageReceiver& obj, Message m, Message* replyPtr)
{
  obj.doSendRequest(m, replyPtr);
}

// send a message list directly to a MessageReceiver.
inline void sendMessages(MessageReceiver& obj, MessageList msgList)
{
  for(auto& m : msgList)
  {
    sendMessage(obj, m);
  }
}

// send a message to a MessageReceiver object through a unique_ptr &,
// like we obtain from a Collection.
template <typename T>
inline void sendMessage(const std::unique_ptr< T > & pObj, Message m)
{
  if(pObj)
    pObj->doSendMessage(m);
}

// send a list of messages to a MessageReceiver object through a Collection.
template <typename T>
inline void sendMessageList(const std::unique_ptr< T > & pObj, MessageList msgList)
{
  if(pObj)
  {
    for(auto& m : msgList)
    {
      pObj->doSendMessage(m);
    }
  }
}


// sending messages to MessageReceivers via Collections

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

inline std::ostream& operator<<(std::ostream& out, const Message& r)
{
  std::cout << "[" << r.address << ": " << r.value << "]";
  return out;
}


}  // namespace ml
