// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLTree.h"

namespace ml
{

template <typename T>
class Collection
{
 public:
  // TODO these types should be protected, but then it becomes hard to write something
  // like inSubCollection() below. revisit
  using ObjectPointerType = std::unique_ptr<T>;
  using TreeType = Tree<ObjectPointerType>;

 protected:
  TreeType* _tree{nullptr};

 public:
  // The non-null constructor needs to be passed a reference to a Tree
  // that holds the actual collection. This allows a collection to refer
  // to a sub-path of another collection. To create
  // the root collection and the tree of objects, use CollectionRoot below.
  Collection(TreeType& t) : _tree(&t) {}

  Collection() = default;
  ~Collection() = default;

  // iterators for range-based for loops. null iterators are compared
  // by value and will equal each other.

  inline typename TreeType::const_iterator begin() const
  {
    const static typename TreeType::const_iterator nullIterator{};
    return _tree ? _tree->begin() : nullIterator;
  }

  inline typename TreeType::const_iterator end() const
  {
    const static typename TreeType::const_iterator nullIterator{};
    return _tree ? _tree->end() : nullIterator;
  }

  const ObjectPointerType& operator[](Path p) const
  {
    static std::unique_ptr<T> _nullObjPtr;
    if (!_tree)
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
    if (!_tree)
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
    if (!_tree) return;
    _tree->add(p, std::move(newVal));
  }

  // create a new object in the collection at the given path, constructed with the remaining
  // arguments.
  template <typename TT, typename... Targs>
  void add_unique(Path p, Targs... Fargs)
  {
    if (!_tree) return;
    _tree->add(p, std::move(std::make_unique<TT>(Fargs...)));
  }

  // like add_unique but also passes the new object an initial argument: the Collection
  // at its own new location. For constructing objects that refer to groups of other objects.
  template <typename TT, typename... Targs>
  void add_unique_with_collection(Path p, Targs... Fargs)
  {
    if (!_tree) return;
    _tree->add(p, ObjectPointerType());
    auto sc = getSubCollection(p);
    _tree->operator[](p) = std::move(std::make_unique<TT>(sc, Fargs...));
  }

  // return the Collection under the given node. Note that this does not
  // include the given node as a member, just as whole Collection does
  // not include a "/" or null-named node.
  inline Collection getSubCollection(Path addr)
  {
    if (!_tree) return Collection<T>();

    TreeType* subTree = _tree->getNode(addr);
    if (subTree)
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

  // return a reference to the Tree under the given node.
  // TODO see if we can make this return const TreeType&
  inline TreeType& subCollReference(Path addr)
  {
    static TreeType nullTree{};
    if (!_tree) return nullTree;

    TreeType* subTree = _tree->getNode(addr);
    if (subTree)
    {
      // return a reference to the subTree.
      return *subTree;
    }
    else
    {
      return nullTree;
    }
  }

  // call some function with each item in the collection.
  // if the currentPathPtr argument is non-null, the Path it points to
  // is updated with the current value before each function call.
  template <typename CALLABLE>
  void forEach(CALLABLE f, Path* currentPathPtr = nullptr)
  {
    if (!_tree) return;
    for (auto it = _tree->begin(); it != _tree->end(); ++it)
    {
      if (currentPathPtr)
      {
        *currentPathPtr = it.getCurrentPath();
      }
      const ObjectPointerType& p = *it;
      f(*p);
    }
  }

  // call some function with each direct child of the collection's root node.
  // if the currentPathPtr argument is non-null, the Path it points to
  // is updated with the current value before each function call.
  template <typename CALLABLE>
  void forEachChild(CALLABLE f, Path* currentPathPtr = nullptr)
  {
    if (!_tree) return;
    // Tree currently offers only depth-first iterators so
    // we have to iterate the entire tree even though we only want children
    // of the root node.
    for(auto it = _tree->begin(); it != _tree->end(); ++it)
    {
      if((it.getCurrentDepth() == 0) && (it.currentNodeHasValue()))
      {
        if(currentPathPtr)
        {
          *currentPathPtr = it.getCurrentPath();
        }
        if(*it)
        {
          const ObjectPointerType& obj = *it;
          f(*obj);
        }
      }
    }
  }

  inline void dump() const
  {
    for (auto it = _tree->begin(); it != _tree->end(); ++it)
    {
      std::cout << it.getCurrentPath() << " [" << *it << "] \n";
    }
  }

  inline size_t size() const { return _tree ? _tree->size() : 0; }
};

// CollectionRoot: a handy subclass to combine a Collection with its Tree
template <typename T>
class CollectionRoot : public Collection<T>
{
  typename Collection<T>::TreeType _localTree;

 public:
  // return collection referring to our internal tree
  CollectionRoot() : Collection<T>(_localTree){};
  ~CollectionRoot() = default;

  inline void clear() { _localTree.clear(); }
};

template <typename T>
inline Collection<T> getSubCollection(Collection<T> coll, Path addr)
{
  return coll.getSubCollection(addr);
}

// return a reference to the Tree in the Collection. Used for sending a subcollection to
// the forEach and forEachChild functions, for example:
// forEach< Widget >(inSubCollection(widgets, "mySubView"), [&](const Widget& w){ /* do something */
// });
template <typename T>
inline typename Collection<T>::TreeType& inSubCollection(Collection<T> coll, Path addr)
{
  return coll.subCollReference(addr);
}

template <typename T, typename CALLABLE>
inline void forEach(Collection<T>& coll, CALLABLE f, Path* currentPathPtr = nullptr)
{
  coll.forEach(f, currentPathPtr);
}

template <typename T, typename CALLABLE>
inline void forEach(typename Collection<T>::TreeType& collRef, CALLABLE f,
                    Path* currentPathPtr = nullptr)
{
  Collection<T> subCollection(collRef);
  subCollection.forEach(f, currentPathPtr);
}

template <typename T, typename CALLABLE>
inline void forEachChild(Collection<T>& coll, CALLABLE f, Path* currentPathPtr = nullptr)
{
  coll.forEachChild(f, currentPathPtr);
}

template <typename T, typename CALLABLE>
inline void forEachChild(typename Collection<T>::TreeType& collRef, CALLABLE f,
                         Path* currentPathPtr = nullptr)
{
  Collection<T> subCollection(collRef);
  subCollection.forEachChild(f, currentPathPtr);
}

}  // namespace ml
