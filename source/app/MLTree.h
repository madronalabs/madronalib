//
//  MLTree.h
//  madronaib
//
//  Created by Randy Jones on 9/22/15.
//
//  A map to a hierarchical container of resources, such as a directory structure.
//
//

#pragma once

#include <map>
#include <vector>
#include <functional>
#include <algorithm>

#include "MLPath.h"
#include "MLTextUtils.h"

// A recursive resource map using Symbol keys, a value class V, and optional comparator class C.
// The value class must have a default constructor V() returning a safe null object.
// Note that this makes (for example) Tree<int> weird to use, because 0 indicates
// a null value. However, we are typically interested in more complex value types like Values or Widgets.
// Heavyweight objects in a Tree should be held by unique_ptrs.

namespace ml{

  template < class V, class C = std::less<Symbol> >
  class Tree final
  {
    // recursive definition: a Tree has a map of Symbols to Trees, and a value.
    using mapT = std::map< Symbol, Tree<V, C>, C >;
    mapT mChildren{};
    V _value{};

  public:
    Tree<V, C>() = default;
    Tree<V, C>(V val) : _value(std::move(val)) { }

    void clear() { mChildren.clear(); _value = V(); }
    bool hasValue() const {  return _value != V(); }
    bool isLeaf() const { return mChildren.size() == 0; }

    // find a tree node at the specified path.
    // if successful, return a pointer to the node. If unsuccessful, return nullptr.
    Tree<V, C>* getNode(Path path)
    {
      auto pNode = this;
      for(Symbol key : path)
      {
        if(pNode->mChildren.find(key) != pNode->mChildren.end())
        {
          pNode = &(pNode->mChildren[key]);
        }
        else
        {
          return nullptr;
        }
      }
      return pNode;
    }

    bool valueExists(Path path)
    {
      return (getNode(path) != nullptr);
    }

    V& getNullValue()
    {
      static V nullValue{};
      return nullValue;
    }

    // if the path exists, returns the value in the tree at the path.
    // else, return a null object of our value type V.
    V& getValue(Path p)
    {
      auto pNode = getNode(p);
      if(pNode)
      {
        return pNode->_value;
      }
      else
      {
        return getNullValue();
      }
    }

    // if the path exists, returns a reference to the value in the tree at the path.
    // else, add a new default object of our value type V.
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

    // add a value V to the Tree such that getValue(path) will return V.
    // add any intermediate nodes necessary in order to put it there.
    Tree<V, C>* add(ml::Path path, V val)
    {
      auto pNode = this;
      int pathSize = path.getSize();
      int pathDepthFound = 0;

      // walk the tree up to, but not including, the last node, as long as branches matching the path are found
      for(Symbol key : path)
      {
        // break if at last node
        if(pathDepthFound >= pathSize - 1) break;

        if(pNode->mChildren.find(key) != pNode->mChildren.end())
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

      // add the remainder of the path to the map, again up to, but not including, the last node
      for(int i = pathDepthFound; i < pathSize - 1; ++i)
      {
        // [] operator creates the new node
        auto newNodeName = path.getElement(i);
        pNode = &(pNode->mChildren[newNodeName]);
      }

      // search for last node
      auto lastNodeName = path.getElement(pathSize - 1);
      if(pNode->mChildren.find(lastNodeName) == pNode->mChildren.end())
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

    // NOTE this iterator does not work with STL algorithms in general, only for simple begin(), end() loops.
    // This is enough to support the range-based for syntax.
    // post-increment(operator++(int)) is not defined. Instead use pre-increment form ++it.

    friend class const_iterator;
    class const_iterator
    {
      std::vector< const Tree<V, C>* > mNodeStack;
      std::vector< typename mapT::const_iterator > mIteratorStack;

    public:
      const_iterator(const Tree<V, C>* p)
      {
        mNodeStack.push_back(p);
        mIteratorStack.push_back(p->mChildren.begin());
      }

      const_iterator(const Tree<V, C>* p, const typename mapT::const_iterator subIter)
      {
        mNodeStack.push_back(p);
        mIteratorStack.push_back(subIter);
      }

      ~const_iterator() {}

      bool operator==(const const_iterator& b) const
      {
        // bail out here if possible.
        if (mNodeStack.size() != b.mNodeStack.size())
          return false;
        if (mNodeStack.back() != b.mNodeStack.back())
          return false;

        // if the containers are the same, we may compare the iterators.
        return (mIteratorStack.back() == b.mIteratorStack.back());
      }

      bool operator!=(const const_iterator& b) const
      {
        return !(*this == b);
      }

      const V& operator*() const
      {
        return ((*mIteratorStack.back()).second)._value;
      }

      bool atEndOfMap() { return (mIteratorStack.back() == (mNodeStack.back())->mChildren.end()); }

      // advance to the next leaf that has a value
      const const_iterator& operator++()
      {
        do
        {
          auto& currentIterator = mIteratorStack.back();
         // bool atEndOfMap = (currentIterator == (mNodeStack.back())->mChildren.end());
          if(!atEndOfMap())
          {
            auto currentChildNodePtr = &((*currentIterator).second);
            if (!currentChildNodePtr->isLeaf())
            {
              // down
              mNodeStack.push_back(currentChildNodePtr);
              mIteratorStack.push_back(currentChildNodePtr->mChildren.begin());
            }
            else
            {
              // across
              currentIterator++;
            }
          }
          else
          {
            if(mNodeStack.size() > 1)
            {
              // up
              mNodeStack.pop_back();
              mIteratorStack.pop_back();
              mIteratorStack.back()++;
            }
            else
            {
              break;
            }
          }
        }
        while(!currentNodeHasValue());

        return *this;
      }
      
      bool currentNodeHasValue() const
      {
        auto parentNode = mNodeStack.back();
        auto& currentIterator = mIteratorStack.back();

        // no value (and currentIterator not dereferenceable!) if at end()
        if(currentIterator == parentNode->mChildren.end()) return false;

        return(((*currentIterator).second).hasValue());
      }

      Symbol getCurrentNodeName() const
      {
        return (*(mIteratorStack.back())).first;
      }

      // return entire path to the current node. If any iterator is not referenceable this will fail.
      Path getCurrentNodePath() const
      {
        Path p;
        for(auto currentIterator : mIteratorStack)
        {
          p.addSymbol((*currentIterator).first);
        }
        return p;
      }

      int getCurrentDepth() const { return mNodeStack.size() - 1; }
    };

    // start at beginning, then advance until a node with a value is reached.
    inline const_iterator begin() const
    {
      auto it = const_iterator(this);
      while(!it.currentNodeHasValue() && !it.atEndOfMap())
      {
        ++it;
      }
      return it;
    }

    inline const_iterator end() const
    {
      return const_iterator(this, mChildren.end());
    }

    // using an iterator, dump only the nodes with values.
    // to visualize all nodes another interface would need to be added, because the iterator
    // only stops on nodes with values.
    inline void dump() const
    {
      for(auto it = begin(); it != end(); ++it)
      {
        std::cout << ml::textUtils::spaceStr(it.getCurrentDepth()) << it.getCurrentNodeName() << " [" << *it << "]\n";
      }
    }
  };
} // namespace ml
