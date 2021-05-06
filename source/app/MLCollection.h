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

template <typename T>
class Collection
{
protected:
  using ObjectPointerType = std::unique_ptr<T>;
  using TreeType = Tree<ObjectPointerType>;

private:
  TreeType* _tree{nullptr};
  
public:
  // The non-null constructor needs to be passed a reference to a Tree
  // that holds the actual collection. This allows a collection to refer
  // to a sub-path of another collection. To create
  // the root collection and the tree of objects, use CollectionRoot below.
  Collection(TreeType& t) : _tree(&t)
  {
    //_isNull = false;
    static_assert(std::is_base_of<Collectable, T>::value,
                  "Collection: object type is not collectable!");
  }

  Collection() = default;
  ~Collection() = default;
  
  // Is the collection null?
  explicit operator bool() {return _tree != nullptr;}
  
  // If a collection might be null (not merely empty but having
  // no Tree), we need to check that before we can use these iterators!
  inline typename TreeType::const_iterator begin() const { return _tree->begin(); }
  inline typename TreeType::const_iterator end() const { return _tree->end(); }

  // the other methods of access following are OK to use with a
  // null Collection.
  const ObjectPointerType& operator[](Path p) const
  {
    static std::unique_ptr<T> _nullObjPtr;
    if(!_tree)
    {
      return _nullObjPtr;
    }
    else
    {
      return _tree->operator[](p);
    }
  }

  // if an object exists at the path, return a unique_ptr to it.
  // otherwise, return a unique_ptr to null.
  const ObjectPointerType& find(Path p) const
  {
    static std::unique_ptr<T> _nullObjPtr;
    if(!_tree)
    {
      return _nullObjPtr;
    }

    const TreeType* n = _tree->getConstNode(p);
    if (n && n->hasValue())
    {
      return n->getValue();
    }
    else
    {
      return _nullObjPtr;
    }
  }

  // add the object referred to by newVal to the collection.
  void add(Path p, T& newVal)
  {
    if(!_tree) return;
    _tree->add(p, std::move(newVal));
  }

  // create a new object in the collection at the given path, constructed with the remaining
  // arguments.
  template <typename TT, typename... Targs>
  void add_unique(Path p, Targs... Fargs)
  {
    if(!_tree) return;
    _tree->add(p, std::move(ml::make_unique<TT>(Fargs...)));
  }

  // like add_unique but also passes an initial argument of the collection
  // at its own new location. This lets objects add to existing collections
  // in their constructors.
  template <typename TT, typename... Targs>
  void add_unique_with_collection(Path p, Targs... Fargs)
  {
    if(!_tree) return;
    _tree->add(p, ObjectPointerType());
    auto subColl = getSubCollection(p);
    _tree->operator[](p) = std::move(ml::make_unique<TT>(subColl, Fargs...));
  }
  
  inline Collection getSubCollection(Path addr)
  {
    if(!_tree) return Collection<T>();

    typename Collection<T>::TreeType* subTree = _tree->getNode(addr);
    if(subTree)
    {
      // return a new Collection referring to the given subTree.
      return Collection<T>(*subTree);
    }
    else
    {
      // if no Tree is found for the Path, the null collection is returned.
      return Collection<T>();
    }
  }

  void forEach(std::function<void(T&)> f) const
  {
    if(!_tree) return;
    for (auto& obj : *_tree)
    {
      f(*obj);
    }
  }
  
  // do something for each direct child of the collection's root node.
  void forEachChild(std::function<void(T&)> f) const
  {
    if(!_tree) return;
    // Tree currently offers only depth-first iterators so
    // we have to iterate the entire tree.
    for (auto it = _tree->begin(); it != _tree->end(); ++it)
    {
      if(it.getCurrentDepth() == 0)
      {
        const ObjectPointerType& obj = *it;
        f(*obj);
      }
    }
  }
  
  void forEachWithPath(std::function<void(T&, Path)> f) const
  {
    if(!_tree) return;
    for (auto it = _tree->begin(); it != _tree->end(); ++it)
    {
      const ObjectPointerType& p = *it;
      f(*p, it.getCurrentNodePath());
    }
  }
  
  void forEachChildWithPath(std::function<void(T&, Path)> f) const
  {
    if(!_tree) return;
    for (auto it = _tree->begin(); it != _tree->end(); ++it)
    {
      if(it.getCurrentDepth() == 0)
      {
        const ObjectPointerType& p = *it;
        f(*p, it.getCurrentNodePath());
      }
    }
  }
  
};

// CollectionRoot: a handy subclass to combine a Collection with its Tree
template< typename T >
class CollectionRoot : public Collection<T>
{
  typename Collection<T>::TreeType _localTree;
  
public:
  // return collection referring to our internal tree
  CollectionRoot() : Collection<T>(_localTree){};
  ~CollectionRoot() = default;
};

template< typename T >
inline Collection< T > getSubCollection(Collection< T > coll, Path addr)
{
  return coll.getSubCollection(addr);
}

template <typename T>
inline void forEach(Collection<T> coll, std::function<void(T&)> f)
{
  coll.forEach(f);
}

template <typename T>
inline void forEachChild(Collection<T> coll, std::function<void(T&)> f)
{
  coll.forEachChild(f);
}

template <typename T>
inline void forEachWithPath(Collection<T> coll, std::function<void(T&, Path)> f)
{
  coll.forEachWithPath(f);
}

template <typename T>
inline void forEachChildWithPath(Collection<T> coll, std::function<void(T&, Path)> f)
{
  coll.forEachChildWithPath(f);
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
inline void sendMessage(Collection<T> coll, Path p, Message m)
{
  if (auto& obj = coll.find(p))
  {
    obj->respond(m);
  }
}

// send a list of messages to an object through a Collection.
template <typename T>
inline void sendMessages(Collection<T> coll, Path p, MessageList msgList)
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
inline void sendMessageToChildren(Collection<T> coll, Message m)
{
  coll.forEachChild([&](T& obj) { obj.respond(m); });
}

template <typename T>
inline void sendMessageToDescendants(Collection<T> coll, Message m)
{
  coll.forEach([&](T& obj) { obj.respond(m); });
}

}  // namespace ml
