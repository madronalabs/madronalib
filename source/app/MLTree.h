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

// A recursive map from Paths to values using Symbol keys, a value class V, and
// optional comparator class C. The value class must have a default constructor
// V() returning a safe null object. Note that this makes (for example)
// Tree<int> weird to use, because 0 indicates a null value. However, we are
// typically interested in more complex value types like Values or Widgets.
// Heavyweight objects in a Tree should be held by unique_ptrs.
//
// Once made, nodes cannot be erased. Typically, the Tree will be made once
// from a parameter list, or rescanned periodically to mirror a file hierarchy.
// Partial deletion would add complexity without a clear benefit.

namespace ml
{

// ============================================================================
// Tree - templated on Key type
// ============================================================================

template <class V, class K = Symbol, class C = std::less<K>>
class Tree
{
  using mapT = std::map<K, Tree<V, K, C>, C>;
  mapT mChildren{};
  V _value{};
  
public:
  Tree<V, K, C>() = default;
  Tree<V, K, C>(V val) : _value(std::move(val)) {}
  
  void clear()
  {
    mChildren.clear();
    _value = V();
  }
  
  void combine(const Tree<V, K, C>& b)
  {
    for (auto it = b.begin(); it != b.end(); ++it)
    {
      add(it.getCurrentPath(), *it);
    }
  }
  
  bool hasValue() const { return _value != V(); }
  const V& getValue() const { return _value; }
  bool isLeaf() const { return mChildren.size() == 0; }
  
  const Tree<V, K, C>* getNode(GenericPath<K> path) const
  {
    auto pNode = this;
    for (K key : path)
    {
      auto it = pNode->mChildren.find(key);
      if (it != pNode->mChildren.end())
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
      auto it = pNode->mChildren.find(key);
      if (it != pNode->mChildren.end())
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
  
  V& operator[](GenericPath<K> p)
  {
    auto pNode = getMutableNode(p);
    if(pNode)
    {
      return pNode->_value;
    }
    else
    {
      return add(p, V())->_value;
    }
  }
  
  const V& operator[](GenericPath<K> p) const
  {
    static const V nullValue{};
    auto pNode = getNode(p);
    if (pNode)
    {
      return pNode->_value;
    }
    else
    {
      return nullValue;
    }
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
    
    for (int i = 0; i < pathSize - 1; ++i)
    {
      pNode = &(pNode->mChildren[path.getElement(i)]);
    }
    
    auto lastNodeName = path.getElement(pathSize - 1);
    auto it = pNode->mChildren.find(lastNodeName);
    if(it != pNode->mChildren.end())
    {
      it->second._value = std::move(val);
      pNode = &(it->second);
    }
    else
    {
      auto [newIt, inserted] = pNode->mChildren.emplace(lastNodeName, std::move(val));
      pNode = &(newIt->second);
    }
    
    return pNode;
  }
  
  friend class const_iterator;
  class const_iterator
  {
    std::vector<const Tree<V, K, C>*> mNodeStack;
    std::vector<typename mapT::const_iterator> mIteratorStack;
    
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const V;
    using difference_type = int;
    using pointer = const V*;
    using reference = const V&;
    
    const_iterator() {}
    
    const_iterator(const Tree<V, K, C>* p, const typename mapT::const_iterator subIter)
    {
      mNodeStack.push_back(p);
      mIteratorStack.push_back(subIter);
    }
    
    ~const_iterator() {}
    
    bool operator==(const const_iterator& b) const
    {
      if (mNodeStack.size() != b.mNodeStack.size()) return false;
      if (mNodeStack.empty() && b.mNodeStack.empty()) return true;
      if (mNodeStack.back() != b.mNodeStack.back()) return false;
      return (mIteratorStack.back() == b.mIteratorStack.back());
    }
    
    bool operator!=(const const_iterator& b) const { return !(*this == b); }
    
    const V& operator*() const
    {
      return ((*mIteratorStack.back()).second)._value;
    }
    
    void push(const Tree<V, K, C>* childNodePtr)
    {
      mNodeStack.push_back(childNodePtr);
      mIteratorStack.push_back(childNodePtr->mChildren.begin());
    }
    
    void pop()
    {
      if(mNodeStack.size() > 1)
      {
        mNodeStack.pop_back();
        mIteratorStack.pop_back();
      }
    }
    
    bool atEndOfMap() const {
      return (mIteratorStack.back() == (mNodeStack.back())->mChildren.end());
    }
    
    bool nextNode()
    {
      auto& currentIterator = mIteratorStack.back();
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
        if(mNodeStack.size() > 1)
        {
          pop();
          mIteratorStack.back()++;
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
      auto& currentIterator = mIteratorStack.back();
      if (!atEndOfMap())
      {
        auto currentChildNodePtr = &((*currentIterator).second);
        if (!currentChildNodePtr->isLeaf())
        {
          push(currentChildNodePtr);
        }
        else
        {
          while(!atEndOfMap())
          {
            currentIterator++;
          }
        }
      }
      else
      {
        currentIterator = mNodeStack.back()->mChildren.begin();
      }
    }
    
    bool hasMoreChildren()
    {
      return ( !atEndOfMap() );
    }
    
    void nextChild()
    {
      mIteratorStack.back()++;
    }
    
    bool currentNodeHasValue() const
    {
      auto parentNode = mNodeStack.back();
      auto& currentIterator = mIteratorStack.back();
      
      if (currentIterator == parentNode->mChildren.end()) return false;
      
      return (((*currentIterator).second).hasValue());
    }
    
    const const_iterator& operator++()
    {
      while(1)
      {
        if(!nextNode()) break;
        if(currentNodeHasValue()) break;
      }
      
      return *this;
    }
    
    size_t getCurrentDepth() const {
      return mNodeStack.size() - 1;
    }
    
    K getCurrentNodeNameAtDepth(size_t i) const
    {
      auto& node = mNodeStack[i];
      auto& iter = mIteratorStack[i];
      if(iter != node->mChildren.end())
      {
        return (*iter).first;
      }
      return K();
    }
    
    K getCurrentNodeName() const
    {
      const size_t stackSize = mNodeStack.size();
      if(stackSize < 1) return K();
      return getCurrentNodeNameAtDepth(stackSize - 1);
    }
    
    GenericPath<K> getCurrentPath() const
    {
      GenericPath<K> p;
      for(int i = 0; i < mNodeStack.size(); ++i)
      {
        K key = getCurrentNodeNameAtDepth(i);
        p.setElement(i, key);
      }
      return p;
    }
    
    void setCurrentPathToRoot()
    {
      mNodeStack.resize(1);
      mIteratorStack.clear();
      mIteratorStack.push_back(mNodeStack[0]->mChildren.end());
    }
    
    bool setCurrentPath(GenericPath<K> p) {
      setCurrentPathToRoot();
      const Tree<V, K, C>* nextNode = mNodeStack[0];
      for(K key : p)
      {
        auto it = nextNode->mChildren.find(key);
        if (it != nextNode->mChildren.end())
        {
          mNodeStack.push_back(nextNode);
          mIteratorStack.push_back(it);
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
    auto it = const_iterator(this, mChildren.begin());
    while (!it.currentNodeHasValue() && !it.atEndOfMap())
    {
      ++it;
    }
    return it;
  }
  
  inline const_iterator beginAtRoot() const
  {
    return const_iterator(this, mChildren.end());
  }
  
  inline const_iterator end() const
  {
    return const_iterator(this, mChildren.end());
  }
  
  inline void dump() const
  {
    size_t maxDepth{0};
    for (auto it = begin(); it != end(); ++it)
    {
      size_t indent = it.getCurrentDepth();
      maxDepth = std::max(maxDepth, indent);
      for(int i=0; i<indent; ++i)
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
      if(!it.atEndOfMap())
      {
        std::cout << it.getCurrentPath();
        if(it.currentNodeHasValue())
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
    for (auto& c : mChildren)
    {
      sum += c.second.size();
    }
    return sum;
  }
};

// Utility functions
template <class V, class K = Symbol, class C = std::less<K>>
bool treeNodeExists(const Tree<V, K, C>& t, GenericPath<K> path)
{
  return (t.getNode(path) != nullptr);
}

template <class V, class K = Symbol, class C = std::less<K>>
bool treeNodeExists(const Tree<V, K, C>& t, const char* pathStr)
{
  return (t.getNode(Path(pathStr)) != nullptr);
}

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

// ============================================================================
// Type aliases for common usage
// ============================================================================

template <class V>
using SymbolTree = Tree<V, Symbol>;

template <class V>
using DynamicTree = Tree<V, TextFragment>;

} // namespace ml
