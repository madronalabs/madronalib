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
// Note that this makes Tree<int> weird to use, because 0 indicates
// a null value. However, we are typically interested in more complex value types like signals or files.

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
    typedef std::map< Symbol, Tree<V, C>, C > mapT;
    mapT mChildren;
    V mValue{};

  public:

    Tree<V, C>() : mChildren(), mValue() { }
    ~Tree<V, C>() {}

    void clear() { mChildren.clear(); }

    const V& getValue() const { return mValue; }
    void setValue(const V& v) { mValue = v; }
    bool hasValue() const {  return mValue != V(); }

    bool isLeaf() const { return mChildren.size() == 0; }

    // find a tree node at the specified path.
    // if successful, return a pointer to the node. If unsuccessful, return nullptr.
    Tree<V, C>* findNode(Path path)
    {
      Tree<V, C>* pNode = this;

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

    // find a value by its path.
    // if the path exists, returns the value in the tree.
    // else, return a null object of our value type V.
    V findValue(Path p)
    {
      Tree<V, C>* pNode = findNode(p);
      if(pNode)
      {
        return pNode->getValue();
      }
      else
      {
        return V();
      }
    }

    /*
     V findValue(const char* pathStr)
     {
     return findValue(ml::Path(pathStr));
     }
     */

    Tree<V, C>* addValue (ml::Path path, const V& val)
    {
      Tree<V, C>* newNode = addNode(path);
      newNode->setValue(val);
      return newNode;
    }

    /*
     Tree<V, C>* addValue (const char* pathStr, const V& val)
     {
     return addValue(ml::Path(pathStr), val);
     }
     */

    // TODO this iterator does not work with STL algorithms in general, only for simple begin(), end() loops.
    // add the other methods needed.

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

      bool nodeHasValue() const
      {
        const Tree<V, C>* parentNode = mNodeStack.back();
        const typename mapT::const_iterator& currentIterator = mIteratorStack.back();

        // no value (and currentIterator not dereferenceable!) if at end()
        if(currentIterator == parentNode->mChildren.end()) return false;

        const Tree<V, C>* currentChildNode = &((*currentIterator).second);
        return(currentChildNode->hasValue());
      }


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
        return ((*mIteratorStack.back()).second).getValue();
      }


      /*
       const Tree<V, C>* operator->() const
       {
       return &((*mIteratorStack.back()).second);
       }
       */

      // advance to the next leaf that has a value
      const const_iterator& operator++()
      {
        while(1)
        {
          typename mapT::const_iterator& currentIterator = mIteratorStack.back();

          std::cout << "nodes: " << mNodeStack.size() << ", iterators: " <<  mIteratorStack.size();

          if(atEndOfMap())
          {
            if(mNodeStack.size() > 1)
            {
              std::cout << " up ";

              // up
              mNodeStack.pop_back();
              mIteratorStack.pop_back();
              mIteratorStack.back()++;
            }
            else
            {
              std::cout << " end ";
              break;
            }
          }
          else
          {
            const Tree<V, C>* currentChildNode = &((*currentIterator).second);
            if (!currentChildNode->isLeaf())
            {
              std::cout << " down ";

              // down
              mNodeStack.push_back(currentChildNode);
              mIteratorStack.push_back(currentChildNode->mChildren.begin());
            }
            else
            {
              std::cout << " across ";

              // across
              currentIterator++;
            }
          }
          if(nodeHasValue()) break;
        }

        std::cout << "\n";

        return *this;

      }
      
      const_iterator& operator++(int)
      {
        this->operator++();
        return *this;
      }

      bool atEndOfMap() const
      {
        const Tree<V, C>* parentNode = mNodeStack.back();
        const typename mapT::const_iterator& currentIterator = mIteratorStack.back();
        return(currentIterator == parentNode->mChildren.end());
      }

      Symbol getLeafName() const
      {
        const Tree<V, C>* parentNode = mNodeStack.back();
        const typename mapT::const_iterator& currentIterator = mIteratorStack.back();

        // no value (and currentIterator not dereferenceable!) if at end()
        if(currentIterator == parentNode->mChildren.end()) return Symbol();

        return (*currentIterator).first;
      }

      int getDepth() { return mNodeStack.size() - 1; }
    };

    inline const_iterator begin() const
    {
      auto it = const_iterator(this);
      while(!it.nodeHasValue())
      {
        it++;
      }

      return it;// const_iterator(this);
    }

    inline const_iterator end() const
    {
      return const_iterator(this, mChildren.end());
    }

    inline void dump() const
    {
      for(auto it = begin(); it != end(); it++)
      {
        std::cout << ml::textUtils::spaceStr(it.getDepth()) << it.getLeafName() << " [" << *it << "]\n";

        /*
         if(it.nodeHasValue())
         {
         std::cout << ml::textUtils::spaceStr(it.getDepth()) << it.getLeafName() << " [" << it->getValue() << "]\n";
         }
         else
         {

         // TODO can use getDepth() to restore the pretty slashes
         std::cout << ml::textUtils::spaceStr(it.getDepth()) << "/" << it.getLeafName() << "\n";
         }
         */

      }
    }

  private:

    // add a map node at the specified path, and any parent nodes necessary in order to put it there.
    // If a node already exists at the path, return the existing node,
    // else return a pointer to the new node.
    Tree<V, C>* addNode(ml::Path path)
    {
      Tree<V, C>* pNode = this;

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
        // [] operator crates the new node
        pNode = &(pNode->mChildren[*it]);
      }

      return pNode;
    }

  };

} // namespace ml

