// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "MLPath.h"
#include "MLValue.h"

// Tree
// --------
//
// Uses:
//
// Tree is a recursive map from Paths to values, used for hierarchical data
// structures like parameter lists, state trees, and file system representations.
// The tree maps GenericPath<K> keys to values of type V.
//
// Types in use:
//   Tree<V, Symbol>                            // (default) For compile-time structures
//   TextTree<V> = Tree<V, TextFragment>    // For runtime structures
//
// Static use case (Tree):
// Synth parameters, DSP graph configurations, and other compile-time-known
// structures benefit from hash-based Symbol keys. Paths can be defined as
// constexpr, and lookups are extremely fast (hash comparison only). No heap
// allocation occurs during lookup if all Symbols are pre-registered.
//
// Dynamic use case (TextTree):
// File system hierarchies, user-generated content, and runtime-discovered
// structures use TextFragment keys. No symbol table overhead, no registration
// cost. Suitable for transient data that doesn't benefit from hash optimization.
//
// Heavyweight objects in a Tree are moved, not copied, thanks to move semantics.
// So it's OK to use most objects as Values.
// Use unique_ptr when you need:
// - Polymorphism (base class pointers to derived objects)
// - Optional ownership (nullptr = no value)
// - Non-copyable resources (file handles, etc.)
//
// Requirements:
//
// Once created, nodes cannot be deleted. The Tree can be cleared entirely, or
// nodes can have their values updated, but partial deletion is not supported.
// This simplifies memory management and thread safety for real-time use.
//
// The value type V must have a default constructor V() that returns a safe
// null object. This allows operator[] to return references even for non-existent
// paths without heap allocation.
//
// Trees can be combined, compared for equality, and iterated over. The iterator
// visits only nodes with values, skipping intermediate nodes.
//
// NOTE: In the future let's aim for STL-compliance and visit every node as a generic
// container should. Use Visitor pattern and Range objects (ValueOnlyRange)
// to iterate over values. Something like
// ValueOnlyRange<V, C> values() const {return ValueOnlyRange<V, C>(this);}
//
// see also: Path, TextPath, GenericPath, Value

namespace ml
{

// Tree - templated on Key type

template <class V, class K = Symbol, class C = std::less<K>>
class Tree
{
  using mapT = std::map<K, Tree<V, K, C>, C>;
  mapT children_{};
  V value_{};

 public:
  Tree<V, K, C>() = default;
  Tree<V, K, C>(V val) : value_(std::move(val)) {}

  void clear()
  {
    children_.clear();
    value_ = V();
  }

  void combine(const Tree<V, K, C>& b)
  {
    for (auto it = b.begin(); it != b.end(); ++it)
    {
      add(it.getCurrentPath(), *it);
    }
  }

  bool hasValue() const { return value_ != V(); }
  const V& getValue() const { return value_; }
  bool isLeaf() const { return children_.size() == 0; }

  const Tree<V, K, C>* getNode(GenericPath<K> path) const
  {
    auto pNode = this;
    for (K key : path)
    {
      auto it = pNode->children_.find(key);
      if (it != pNode->children_.end())
      {
        pNode = &(it->second);
      }
      else
      {
        return nullptr;
      }
    }
    return pNode;
  }


  Tree<V, K, C>* getMutableNode(GenericPath<K> path)
  {
    auto pNode = this;
    for (K key : path)
    {
      auto it = pNode->children_.find(key);
      if (it != pNode->children_.end())
      {
        pNode = &(it->second);
      }
      else
      {
        return nullptr;
      }
    }
    return pNode;
  }

  // operator[] from string literals registers symbols in the Path
  V& operator[](const char* pathStr) { return operator[](runtimePath(pathStr)); }

  V& operator[](GenericPath<K> p)
  {
    auto pNode = getMutableNode(p);
    if (pNode)
    {
      return pNode->value_;
    }
    else
    {
      return add(p, V())->value_;
    }
  }

  const V& operator[](GenericPath<K> p) const
  {
    static const V nullValue{};
    auto pNode = getNode(p);
    if (pNode)
    {
      return pNode->value_;
    }
    else
    {
      return nullValue;
    }
  }
  
  const V& getValueFromHash(HashPath path) const
  {
    static const V nullValue{};
    auto pNode = this;
    for(int i=0; i<path.size_; ++i)
    {
      // use Symbol ctor from hash
      Symbol key(path.elements_[i]);

      auto it = pNode->children_.find(key);
      if (it != pNode->children_.end())
      {
        pNode = &(it->second);
      }
      else
      {
        return nullValue;
      }
    }
    return pNode->value_;
  }

  inline bool operator==(const Tree<V, K, C>& b) const
  {
    auto itA = begin();
    auto itB = b.begin();
    for (; (itA != end()) && (itB != b.end()); ++itA, ++itB)
    {
      if (itA.getCurrentNodeName() != itB.getCurrentNodeName())
      {
        return false;
      }
      if (*itA != *itB)
      {
        return false;
      }
    }
    return (itA == end()) && (itB == b.end());
  }

  inline bool operator!=(const Tree<V, K, C>& b) const { return !(operator==(b)); }

  Tree<V, K, C>* add(GenericPath<K> path, V val)
  {
    auto pNode = this;
    int pathSize = path.getSize();
    if (!pathSize) return pNode;

    for (int i = 0; i < pathSize - 1; ++i)
    {
      pNode = &(pNode->children_[path.getElement(i)]);
    }

    auto lastNodeName = path.getElement(pathSize - 1);
    auto it = pNode->children_.find(lastNodeName);
    if (it != pNode->children_.end())
    {
      it->second.value_ = std::move(val);
      pNode = &(it->second);
    }
    else
    {
      auto [newIt, inserted] = pNode->children_.emplace(lastNodeName, std::move(val));
      pNode = &(newIt->second);
    }

    return pNode;
  }

  friend class const_iterator;
  class const_iterator
  {
    std::vector<const Tree<V, K, C>*> nodeStack_;
    std::vector<typename mapT::const_iterator> iteratorStack_;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const V;
    using difference_type = int;
    using pointer = const V*;
    using reference = const V&;

    const_iterator() {}

    const_iterator(const Tree<V, K, C>* p, const typename mapT::const_iterator subIter)
    {
      nodeStack_.push_back(p);
      iteratorStack_.push_back(subIter);
    }

    ~const_iterator() {}

    bool operator==(const const_iterator& b) const
    {
      if (nodeStack_.size() != b.nodeStack_.size()) return false;
      if (nodeStack_.empty() && b.nodeStack_.empty()) return true;
      if (nodeStack_.back() != b.nodeStack_.back()) return false;
      return (iteratorStack_.back() == b.iteratorStack_.back());
    }

    bool operator!=(const const_iterator& b) const { return !(*this == b); }

    const V& operator*() const { return ((*iteratorStack_.back()).second).value_; }

    void push(const Tree<V, K, C>* childNodePtr)
    {
      nodeStack_.push_back(childNodePtr);
      iteratorStack_.push_back(childNodePtr->children_.begin());
    }

    void pop()
    {
      if (nodeStack_.size() > 1)
      {
        nodeStack_.pop_back();
        iteratorStack_.pop_back();
      }
    }

    bool atEndOfMap() const
    {
      return (iteratorStack_.back() == (nodeStack_.back())->children_.end());
    }

    bool nextNode()
    {
      auto& currentIterator = iteratorStack_.back();
      if (!atEndOfMap())
      {
        auto currentChildNodePtr = &((*currentIterator).second);
        if (!currentChildNodePtr->isLeaf())
        {
          push(currentChildNodePtr);
        }
        else
        {
          currentIterator++;
        }
      }
      else
      {
        if (nodeStack_.size() > 1)
        {
          pop();
          iteratorStack_.back()++;
        }
        else
        {
          return 0;
        }
      }
      return 1;
    }

    void firstChild()
    {
      auto& currentIterator = iteratorStack_.back();
      if (!atEndOfMap())
      {
        auto currentChildNodePtr = &((*currentIterator).second);
        if (!currentChildNodePtr->isLeaf())
        {
          push(currentChildNodePtr);
        }
        else
        {
          while (!atEndOfMap())
          {
            currentIterator++;
          }
        }
      }
      else
      {
        currentIterator = nodeStack_.back()->children_.begin();
      }
    }

    bool hasMoreChildren() { return (!atEndOfMap()); }

    void nextChild() { iteratorStack_.back()++; }

    bool currentNodeHasValue() const
    {
      auto parentNode = nodeStack_.back();
      auto& currentIterator = iteratorStack_.back();

      if (currentIterator == parentNode->children_.end()) return false;

      return (((*currentIterator).second).hasValue());
    }

    const const_iterator& operator++()
    {
      while (1)
      {
        if (!nextNode()) break;
        if (currentNodeHasValue()) break;
      }

      return *this;
    }

    size_t getCurrentDepth() const { return nodeStack_.size() - 1; }

    K getCurrentNodeNameAtDepth(size_t i) const
    {
      auto& node = nodeStack_[i];
      auto& iter = iteratorStack_[i];
      if (iter != node->children_.end())
      {
        return (*iter).first;
      }
      return K();
    }

    K getCurrentNodeName() const
    {
      const size_t stackSize = nodeStack_.size();
      if (stackSize < 1) return K();
      return getCurrentNodeNameAtDepth(stackSize - 1);
    }

    GenericPath<K> getCurrentPath() const
    {
      GenericPath<K> p;
      for (int i = 0; i < nodeStack_.size(); ++i)
      {
        K key = getCurrentNodeNameAtDepth(i);
        p.setElement(i, key);
      }
      return p;
    }

    void setCurrentPathToRoot()
    {
      nodeStack_.resize(1);
      iteratorStack_.clear();
      iteratorStack_.push_back(nodeStack_[0]->children_.end());
    }

    bool setCurrentPath(GenericPath<K> p)
    {
      setCurrentPathToRoot();
      const Tree<V, K, C>* nextNode = nodeStack_[0];
      for (K key : p)
      {
        auto it = nextNode->children_.find(key);
        if (it != nextNode->children_.end())
        {
          nodeStack_.push_back(nextNode);
          iteratorStack_.push_back(it);
          nextNode = &(it->second);
        }
        else
        {
          setCurrentPathToRoot();
          return false;
        }
      }
      return true;
    }
  };

  inline const_iterator begin() const
  {
    auto it = const_iterator(this, children_.begin());
    while (!it.currentNodeHasValue() && !it.atEndOfMap())
    {
      ++it;
    }
    return it;
  }

  inline const_iterator beginAtRoot() const { return const_iterator(this, children_.end()); }

  inline const_iterator end() const { return const_iterator(this, children_.end()); }

  inline void dump() const
  {
    size_t maxDepth{0};
    for (auto it = begin(); it != end(); ++it)
    {
      size_t indent = it.getCurrentDepth();
      maxDepth = std::max(maxDepth, indent);
      for (int i = 0; i < indent; ++i)
      {
        std::cout << " ";
      }
      std::cout << it.getCurrentPath() << " [" << *it << "] \n";
    }
    std::cout << "(max depth: " << maxDepth << ")\n";
  }

  inline void dumpAllNodes() const
  {
    for (auto it = beginAtRoot(); it != end(); it.nextNode())
    {
      if (!it.atEndOfMap())
      {
        std::cout << it.getCurrentPath();
        if (it.currentNodeHasValue())
        {
          std::cout << " [" << *it << "] ";
        }
        std::cout << "\n";
      }
    }
  }

  inline size_t size() const
  {
    size_t sum{hasValue()};
    for (auto& c : children_)
    {
      sum += c.second.size();
    }
    return sum;
  }
};

// Utility functions

template <class V, class K = Symbol, class C = std::less<K>>
const Tree<V, K, C> filterByPathList(const Tree<V, K, C>& t, std::vector<GenericPath<K>> pList)
{
  Tree<V, K, C> filteredTree;
  for (auto it = t.begin(); it != t.end(); ++it)
  {
    auto p = it.getCurrentPath();
    if (std::find(pList.begin(), pList.end(), p) != pList.end())
    {
      filteredTree[p] = (*it);
    }
  }
  return filteredTree;
}

// Type aliases for common usage

template <class V>
using TextTree = Tree<V, TextFragment, textUtils::Collator>;

}  // namespace ml
