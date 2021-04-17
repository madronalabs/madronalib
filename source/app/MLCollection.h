// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLMessage.h"
#include "MLTree.h"

namespace ml
{
// To be a member of a Collection, an object must be Collectable.
struct Collectable
{
  // bool conversion: is this object non-null? the default is yes.
  virtual operator bool() const { return true; }

  // Collectable objects must be able to receive messages.
  virtual Value respond(Message m) = 0;
};

// A concrete null object for any Collectable type.
template <typename T>
struct NullCollectableForClass final : public T
{
  // converting this object to bool will return false.
  operator bool() const override { return false; }

  // Sending a null object a message must do nothing.
  Value respond(Message m) override { return false; }
};

template <typename T>
class Collection
{
 protected:
  using ValueType = T&;
  using ObjectPointerType = std::unique_ptr<T>;
  using TreeType = Tree<ObjectPointerType>;
  TreeType& _tree;

  TreeType& getSubTree(Path addr) const { return *_tree.getNode(addr); }

 public:
  // The constructor needs to be passed a reference to a Tree holding the actual collection.
  // This allows a collection to refer to a sub-path of another collection. To create
  // the root collection and the tree of objects, use CollectionRoot below.
  Collection(TreeType& t) : _tree(t)
  {
    static_assert(std::is_base_of<Collectable, T>::value,
                  "Collection: object type is not collectable!");
  }

  ~Collection() = default;

  const ObjectPointerType& operator[](Path p) const { return _tree.operator[](p); }

  // if an object exists at the path, return a unique_ptr to it.
  // otherwise, return a unique_ptr to null.
  const ObjectPointerType& find(Path p) const
  {
    static std::unique_ptr<T> nullPtr{};
    const TreeType* n = _tree.getConstNode(p);
    if (n && n->hasValue())
    {
      return n->getValue();
    }
    else
    {
      return nullPtr;
    }
  }

  // add the object referred to by newVal to the collection.
  void add(Path p, T& newVal) { _tree.add(p, std::move(newVal)); }

  // create a new object in the collection at the given path, constructed with the remaining
  // arguments.
  template <typename TT, typename... Targs>
  void add_unique(Path p, Targs... Fargs)
  {
    _tree.add(p, std::move(ml::make_unique<TT>(Fargs...)));
  }

  // like add_unique but also passes an initial argument of the collection
  // at its own new location. This lets objects add to existing collections
  // in their constructors.
  template <typename TT, typename... Targs>
  void add_unique_with_collection(Path p, Targs... Fargs)
  {
    _tree.add(p, ObjectPointerType());
    auto subColl = getSubCollection(*this, p);
    _tree[p] = std::move(ml::make_unique<TT>(subColl, Fargs...));
  }

  inline typename TreeType::const_iterator begin() const { return _tree.begin(); }
  inline typename TreeType::const_iterator end() const { return _tree.end(); }

  friend inline Collection<T> getSubCollection(Collection<T>& coll, Path addr)
  {
    return Collection<T>(coll.getSubTree(addr));
  }

  void forEach(std::function<void(T&)> f)
  {
    for (auto& obj : _tree)
    {
      f(*obj);
    }
  }
  
  // do something for each child of the collection's root node.
  void forEachChild(std::function<void(T&)> f)
  {
    // Tree currently offers only depth-first iterators so
    // we have to iterate the entire tree.
    for (auto it = _tree.begin(); it != _tree.end(); ++it)
    {
      if(it.getCurrentDepth() == 0)
      {
        const ObjectPointerType& obj = *it;
        f(*obj);
      }
    }
  }
  void forEachPath(std::function<void(Path)> f)
  {
    for (auto it = _tree.begin(); it != _tree.end(); ++it)
    {
      f(it.getCurrentNodePath());
    }
  }
  void forEachPathAndObject(std::function<void(Path, T&)> f)
  {
    for (auto it = _tree.begin(); it != _tree.end(); ++it)
    {
      const ObjectPointerType& p = *it;
      f(it.getCurrentNodePath(), *p);
    }
  }
};

template <typename T>
class CollectionRoot : public Collection<T>
{
  typename Collection<T>::TreeType _rootObjects;

 public:
  // no collection, create one
  CollectionRoot() : Collection<T>(_rootObjects){};
  ~CollectionRoot() = default;
};

/* untested
template< typename T >
Collection< T > getSubCollection(const Collection< T >& coll, std::function< bool(Path) > pathFn)
{
  CollectionRoot< T > newColl{};

  for(auto it = coll._tree.begin(); it != coll._tree.end(); ++it)
  {
    if(*it->hasValue())
    {
      const Path p = it.getCurrentNodePath();
      if(pathFn(p))
      {
        newColl.add(p, *it->getValue());
      }
    }
  }
}
*/

template <typename T>
inline void forEach(Collection<T>& coll, std::function<void(T&)> f)
{
  coll.forEach(f);
}

template <typename T>
inline void forEachConst(Collection<T>& coll, std::function<void(const T&)> f)
{
  coll.forEach(f);
}

template <typename T>
inline void forEachChild(Collection<T>& coll, std::function<void(T&)> f)
{
  coll.forEachChild(f);
}

template <typename T>
inline void forEachPath(Collection<T>& coll, std::function<void(Path)> f)
{
  coll.forEachPath(f);
}

template <typename T>
inline void forEachPathAndObject(Collection<T>& coll, std::function<void(Path, T&)> f)
{
  coll.forEachPathAndObject(f);
}

// send a message directly to a Collectable object.
inline void sendMessage(Collectable& obj, Message m) { obj.respond(m); }

// send a message list directly to a Collectable object.
inline void sendMessages(Collectable& obj, MessageList msgList)
{
  for (auto& m : msgList)
  {
    obj.respond(m);
  }
}

// send a message to a Collectable object through a Collection.
template <typename T>
inline void sendMessage(Collection<T>& coll, Path p, Message m)
{
  if (auto& obj = coll.find(p))
  {
    obj->respond(m);
  }
}

// send a list of messages to an object through a Collection.
template <typename T>
inline void sendMessages(Collection<T>& coll, Path p, MessageList msgList)
{
  if (auto& obj = coll.find(p))
  {
    for (auto& m : msgList)
    {
      obj->respond(m);
    }
  }
}

template <typename T>
inline void broadcastMessage(Collection<T>& coll, Message m)
{
  coll.forEach([&](T& obj) { obj.respond(m); });
}

}  // namespace ml
