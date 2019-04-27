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
// a null value. However, we are typically interested in more complex value types like signals or files.
// heavyweight objects in a Tree should be held by unique_ptrs.

// notes:
// some use cases:
// - tree of Procs (with multicontainer / polyphonic functionality?) - make V = std::vector< Proc >.
//      A path to poly procs would nbeed to be superscripted with the copy# at each node. Each poly node along the way
// would multiply the size of all subnode vectors.
// - key/value store as in Model
// - tree of UI Widgets
// - tree of Files

namespace ml{

  template < class V, class C = std::less<Symbol> >
  class Tree
  {
    // recursive definition: a Tree has a map of Symbols to Trees, and a value.
    using mapT = std::map< Symbol, Tree<V, C>, C >;
    mapT mChildren{};
    V _value{};

  public:
    Tree<V, C>() = default;
    ~Tree<V, C>() = default;

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
          pNode = nullptr;
          break;
        }
      }
      return pNode;
    }

    // if the path exists, returns the value in the tree at the path.
    // else, return a null object of our value type V.
    V getValue(Path p)
    {
      auto pNode = getNode(p);
      if(pNode)
      {
        return pNode->_value;
      }
      else
      {
        return V();
      }
    }

    Tree<V, C>* addValue (ml::Path path, const V& val)
    {
      auto newNode = addNode(path);
      newNode->_value = val;
      return newNode;
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

      // advance to the next leaf that has a value
      const const_iterator& operator++()
      {
        do
        {
          auto& currentIterator = mIteratorStack.back();

          if(!atEndOfMap(currentIterator))
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

      bool atEndOfMap(const typename mapT::const_iterator& currentIterator) const
      {
        return(currentIterator == (mNodeStack.back())->mChildren.end());
      }

      Symbol getCurrentNodeName() const
      {
        const Tree<V, C>* parentNode = mNodeStack.back();
        const typename mapT::const_iterator& currentIterator = mIteratorStack.back();

        // no value (and currentIterator not dereferenceable!) if at end()
        if(currentIterator == parentNode->mChildren.end()) return Symbol();

        return (*currentIterator).first;
      }

      int getCurrentDepth() const { return mNodeStack.size() - 1; }
    };

    // start at beginning, then advance until a node with a value is reached.
    inline const_iterator begin() const
    {
      auto it = const_iterator(this);
      while(!it.currentNodeHasValue())
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
    // to dump all nodes another API could be added, but it this really needed?
    inline void dump() const
    {
      for(auto it = begin(); it != end(); ++it)
      {
        std::cout << ml::textUtils::spaceStr(it.getCurrentDepth()) << it.getCurrentNodeName() << " [" << *it << "]\n";
      }
    }

  private:

    // add a map node at the specified path, and any parent nodes necessary in order to put it there.
    // If a node already exists at the path, return the existing node, else return a pointer to the new node.

    Tree<V, C>* addNode(ml::Path path)
    {
      auto pNode = this;
      int pathDepthFound = 0;

      // walk the path as long as branches are found in the map
      for(Symbol key : path)
      {
        if(pNode->mChildren.find(key) != pNode->mChildren.end())
        {
          pNode = &(pNode->mChildren[key]);
          pathDepthFound++;
        }
        else
        {
          break;
        }
      }

      // add the remainder of the path to the map.
      for(auto it = path.begin() + pathDepthFound; it != path.end(); ++it)
      {
        // [] operator creates the new node
        pNode = &(pNode->mChildren[*it]);
      }

      return pNode;
    }
  };

} // namespace ml

