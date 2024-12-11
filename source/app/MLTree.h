// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2025 Madrona Labs LLC. http://www.madronalabs.com
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

namespace ml
{
template <class V, class C = std::less<Symbol> >
class Tree
{
  // recursive definition: a Tree has a map of Symbols to Trees, and a value.
  using mapT = std::map<Symbol, Tree<V, C>, C>;
  mapT mChildren{};
  V _value{};

 public:
  Tree<V, C>() = default;
  Tree<V, C>(V val) : _value(std::move(val)) {}

  void clear()
  {
    mChildren.clear();
    _value = V();
  }

  void combine(const Tree<V, C>& b)
  {
    for (auto it = b.begin(); it != b.end(); ++it)
    {
      add(it.getCurrentPath(), *it);
    }
  }

  bool hasValue() const { return _value != V(); }
  const V& getValue() const { return _value; }
  bool isLeaf() const { return mChildren.size() == 0; }

  // find a tree node at the specified path.
  // if successful, return a const pointer to the node. If unsuccessful, return nullptr.
  const Tree<V, C>* getConstNode(Path path) const
  {
    auto pNode = this;
    for (Symbol key : path)
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

  // find a tree node at the specified path.
  // if successful, return a pointer to the node. If unsuccessful, return nullptr.
  Tree<V, C>* getNode(Path path) const
  {
    return const_cast<Tree<V, C>*>(const_cast<const Tree<V, C>*>(this)->getConstNode(path));
  }

  // if the path exists, returns a reference to the value in the tree at the
  // path. else, add a new default object of our value type V.
  V& operator[](Path p)
  {
    auto pNode = getNode(p);
    if(pNode)
    {
      return pNode->_value;
    }
    else
    {
      return add(p, V())->_value;
    }
  }

  // if the path exists, returns a const reference to the value in the tree at
  // the path. Otherwise, reference to a null valued object is returned.
  const V& operator[](Path p) const
  {
    static V nullValue{};
    auto pNode = getConstNode(p);
    if (pNode)
    {
      return pNode->_value;
    }
    else
    {
      return nullValue;
    }
  }

  // compare two Trees by value.
  inline bool operator==(const Tree<V, C>& b) const
  {
    auto itA = begin();
    auto itB = b.begin();
    for (; (itA != end()) && (itB != b.end()); ++itA, ++itB)
    {
      // compare node names
      if (itA.getCurrentNodeName() != itB.getCurrentNodeName())
      {
        return false;
      }

      // compare values
      if (*itA != *itB)
      {
        return false;
      }
    }

    // cover the case where one iterator bailed out early
    return (itA == end()) && (itB == b.end());
  }

  inline bool operator!=(const Tree<V, C>& b) const { return !(operator==(b)); }

  // write a value V to the Tree such that getValue(path) will return V.
  // add any intermediate nodes necessary in order to put it there.
  // a pointer to the existing or new tree node is returned.
  Tree<V, C>* add(Path path, V val)
  {
    auto pNode = this;
    int pathSize = path.getSize();
    int pathDepthFound = 0;

    // walk the tree up to, but not including, the last node, as long as
    // branches matching the path are found
    for (Symbol key : path)
    {
      // break if at last node
      if (pathDepthFound >= pathSize - 1) break;

      if (pNode->mChildren.find(key) != pNode->mChildren.end())
      {
        pNode = &(pNode->mChildren[key]);
        pathDepthFound++;
      }
      else
      {
        // break if not found
        break;
      }
    }

    // add the remainder of the path to the map, again up to, but not including,
    // the last node
    for (int i = pathDepthFound; i < pathSize - 1; ++i)
    {
      // [] operator creates the new node
      auto newNodeName = path.getElement(i);
      pNode = &(pNode->mChildren[newNodeName]);
    }

    // search for last node
    auto lastNodeName = path.getElement(pathSize - 1);
    if (pNode->mChildren.find(lastNodeName) == pNode->mChildren.end())
    {
      // if last node does not exist, emplace new value
      pNode->mChildren.emplace(lastNodeName, std::move(val));
    }
    else
    {
      // overwrite existing value using std::move
      // this allows the value to be some unique_ptr<stuff> .
      pNode->mChildren[lastNodeName]._value = std::move(val);
    }

    pNode = &(pNode->mChildren[lastNodeName]);
    return pNode;
  }

  void erase(Path p)
  {
    // TODO
  }

  // NOTE this iterator does not work with STL algorithms in general, only for
  // simple begin(), end() loops. This is enough to support the range-based for
  // syntax. post-increment(operator++(int)) is not defined. Instead use
  // pre-increment form ++it.

  friend class const_iterator;
  class const_iterator
  {
    std::vector<const Tree<V, C>*> mNodeStack;
    std::vector<typename mapT::const_iterator> mIteratorStack;

   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const V;
    using difference_type = int;
    using pointer = const V*;
    using reference = const V&;

    // null iterator that can be returned so begin() = end() when there is no container
    const_iterator() {}
    
    const_iterator(const Tree<V, C>* p, const typename mapT::const_iterator subIter)
    {
      mNodeStack.push_back(p);
      mIteratorStack.push_back(subIter);
    }

    ~const_iterator() {}

    bool operator==(const const_iterator& b) const
    {

      // bail out here if possible.
      if (mNodeStack.size() != b.mNodeStack.size()) return false;

      // check for empty iterators
      if (mNodeStack.empty() && b.mNodeStack.empty()) return true;

      // if the containers are the same, we may compare the iterators.
      if (mNodeStack.back() != b.mNodeStack.back()) return false;
      return (mIteratorStack.back() == b.mIteratorStack.back());
    }

    bool operator!=(const const_iterator& b) const { return !(*this == b); }

    const V& operator*() const
    {
      return ((*mIteratorStack.back()).second)._value;
      
    }
    
    void push(const Tree<V, C>* childNodePtr)
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
    
    
    /*
    // return true if at root.
    bool atRoot() const
    {
      return (mIteratorStack.size() == 1)
      
    }
*/
    
    // return true if at the end of the current submap.
    bool atEndOfMap() const {
      return (mIteratorStack.back() == (mNodeStack.back())->mChildren.end());
    }

    // advance to the next node. Return false if at end of entire tree.
    bool nextNode()
    {
/*
      if(atRoot())
      {
        mIteratorStack.push_back(mNodeStack[0]->mChildren.begin());
        return 1;
      }
*/
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
    
    // go to the first child of the current parent node, or if at end of map,
    // reset to beginning. The reset clause is weird but makes starting from
    // root work properly.
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
          // this node has no children! go to end so that hasMoreChildren() will return false.
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
    
    // will nextChild() iterate to more children?
    bool hasMoreChildren()
    {
      return ( !atEndOfMap() );
    }
    
    // advance to the next child of the current parent node.
    void nextChild()
    {
      mIteratorStack.back()++;
    }
    
    bool currentNodeHasValue() const
    {
      auto parentNode = mNodeStack.back();
      auto& currentIterator = mIteratorStack.back();
      
      // no value (and currentIterator not dereferenceable!) if at end()
      if (currentIterator == parentNode->mChildren.end()) return false;
      
      return (((*currentIterator).second).hasValue());
    }
    
    // advance to the next leaf that has a value
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
    
    Symbol getCurrentNodeNameAtDepth(size_t i) const
    {
      auto& node = mNodeStack[i];
      auto& iter = mIteratorStack[i];
      if(iter != node->mChildren.end())
      {
        return (*iter).first;
      }
      return Symbol();
    }
    
    // return the last symbol of the current node path.
    Symbol getCurrentNodeName() const
    {
      const size_t stackSize = mNodeStack.size();
      if(stackSize < 1) return Symbol();
      return getCurrentNodeNameAtDepth(stackSize - 1);
    }

    // return entire path to the current node. If any iterator is not
    // referenceable this will fail.
    Path getCurrentPath() const
    {
      Path p;
      for(int i = 0; i < mNodeStack.size(); ++i)
      {
        p = Path{p, (getCurrentNodeNameAtDepth(i))};
      }
      return p;
    }
    
    // sets path to root, after which firstChild() will go to the first node in the map.
    // NOTE: from root, nextNode() will not work. TODO fix.
    void setCurrentPathToRoot()
    {
      mNodeStack.resize(1);
      mIteratorStack.clear();
      mIteratorStack.push_back(mNodeStack[0]->mChildren.end());
    }
    
    // Try to set current node to the path p. Return true if successful.
    // If unsuccessful the current path is set to root.
    bool setCurrentPath(Path p) {
      setCurrentPathToRoot();
      const Tree<V, C>* nextNode = mNodeStack[0];
      for(Symbol key : p)
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

  // start at beginning, then advance until a node with a value is reached.
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

  // visit all nodes and dump only the nodes with values.
  inline void dump() const
  {
    for (auto it = begin(); it != end(); ++it)
    {
      std::cout << it.getCurrentPath() << " [" << *it << "] \n";
    }
  }
  
  // visit and dump each node once, including non-leaf nodes.
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
    size_t sum{hasValue()};  // me
    for (auto& c : mChildren)
    {
      sum += c.second.size();
    }
    return sum;
  }
};

// utilities
template <class V, class C = std::less<Symbol> >
bool treeNodeExists(const Tree<V, C>& t, Path path)
{
  return (t.getConstNode(path) != nullptr);
}

template <class V, class C = std::less<Symbol> >
const Tree<V, C> filterByPathList(const Tree<V, C>& t, std::vector<Path> pList)
{
  Tree<V, C> filteredTree;
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

}  // namespace ml
