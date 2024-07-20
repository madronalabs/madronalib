// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// a unit test made using the Catch framework in catch.hpp / tests.cpp.

#include "catch.hpp"
#include "madronalib.h"

using namespace ml;

struct Receiver : public MessageReceiver
{
  Receiver() : value(0){};
  Receiver(int v) : value(v){};
  virtual ~Receiver() = default;
  operator int() const { return value; }
  int value;
  
  void handleMessage(Message m, MessageList*) { /* std::cout << "normal: " << value << "\n";*/ }
};

struct FancyReceiver : public Receiver
{
  FancyReceiver() : Receiver() {}
  FancyReceiver(int v) : Receiver(v) {}
  ~FancyReceiver() = default;
  
  void handleMessage(Message m, MessageList*) { /*std::cout << "fancy!! " << value << " " << m << "\n";*/ }
};


// Receiver with access to the collection it is in, from the root node
// where it it added. This lets objects manage objects in the collection under
// them.
//
// what's this good for? one example is a View class that wants to draw and manipulate
// part of an object hierarchy, but does not own the object data itself.
struct ReceiverWithCollection : public Receiver
{
  ReceiverWithCollection(ml::Collection< Receiver > t, int v) : Receiver(v), _subCollection(t) {}
  ~ReceiverWithCollection() = default;
  
  ml::Collection< Receiver > _subCollection;
};

TEST_CASE("madronalib/core/message", "[message]")
{
  // a null path
  Path p;
  REQUIRE(!p);

  // a null message
  Message nullMessage;
  REQUIRE(!nullMessage);
  
  CollectionRoot< Receiver > myColl;
 
  myColl.add_unique< Receiver >("a", 3);
  myColl.add_unique< FancyReceiver >("a/b", 99);
  myColl.add_unique< Receiver >("j", 4);
  myColl.add_unique< Receiver >("k", 8);
//  myColl.add_unique_with_collection< ReceiverWithCollection >("a/b/c", 42);
  myColl.add_unique< Receiver >("a/b/c", 42);
  myColl.add_unique< FancyReceiver >("a/b/c/d", 5);
  myColl.add_unique< FancyReceiver >("a/b/c/d/e", 6);
  myColl.add_unique< Receiver >("a/b/c/f", 7);
  
  // TODO real tests
  /*
  sendMessage(myColl["a/b/c/f"], {"hello"});
  
  // sending to a null object should not crash
  sendMessage(myColl["nonexistent"], {"hello"});
  
  

  Collection< Receiver >abcColl = getSubCollection(myColl, "a/b/c");
  sendMessageToEach< Receiver >(abcColl, {"hello"});
  std::cout << "\n\n";

  sendMessageToEach< Receiver >(inSubCollection(myColl, "a/b/c"), {"hello"});
  std::cout << "\n\n";
  
  sendMessageToEach< Receiver >(myColl, {"hello"});
  std::cout << "\n\n";
  
  MessageList mla;
  mla.push_back({"foo"});
  
  MessageList mlb;
  mlb.push_back({"bar"});
  
  mla.append(mlb);
  */

}
