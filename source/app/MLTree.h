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

// TODO why not just call this ResourceTree, or even Tree?
// TODO hooks for change callbacks and undoable actions using an undo manager

// notes:
// some use cases:
// - tree of Procs (with multicontainer / polyphonic functionality?) - make V = std::vector< Proc >.
// 	   A path to poly procs would nbeed to be superscripted with the copy# at each node. Each poly node along the way 
// would multiply the size of all subnode vectors.
// - key/value store as in Model 
// - tree of UI Widgets
// - tree of Files

namespace ml{

template < class V, class C = std::less<Symbol> >
class Tree
{
public:
	typedef std::map< Symbol, Tree<V, C>, C > mapT;

	Tree<V, C>() : mChildren(), mValue() { }
	~Tree<V, C>() {}
	
	void clear() { mChildren.clear(); }
	const V& getValue() const { return mValue; }
	void setValue(const V& v) { mValue = v; }
	bool hasValue() const {  return mValue != V(); } 
	bool isLeaf() const { return mChildren.size() == 0; }
	
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
	
	V findValue(const char* pathStr)
	{
		return findValue(ml::Path(pathStr));
	}
	
	Tree<V, C>* addValue (ml::Path path, const V& val)
	{
		Tree<V, C>* newNode = addNode(path);
		newNode->setValue(val);
		return newNode;
	}
	
	Tree<V, C>* addValue (const char* pathStr, const V& val)
	{
		return addValue(ml::Path(pathStr), val);
	}
	
	// TODO this iterator does not work with STL algorithms in general, only for simple begin(), end() loops.
	// add the other methods needed.
	
	friend class const_iterator;
	class const_iterator
	{
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
				
		const Tree<V, C>& operator*() const 
		{ 
			return ((*mIteratorStack.back()).second); 
		}
		
		const Tree<V, C>* operator->() const 
		{ 
			return &((*mIteratorStack.back()).second); 
		}
		
		const const_iterator& operator++()
		{			
			typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			
			if(atEndOfMap())
			{
				if(mNodeStack.size() > 1)
				{
					// up
					mNodeStack.pop_back();
					mIteratorStack.pop_back();
					mIteratorStack.back()++;
				}
			}			
			else
			{
				const Tree<V, C>* currentChildNode = &((*currentIterator).second);
				if (!currentChildNode->isLeaf())
				{
					// down
					mNodeStack.push_back(currentChildNode);
					mIteratorStack.push_back(currentChildNode->mChildren.begin());
				}
				else
				{
					// across
					currentIterator++;
				}
			}
			 
			return *this;
		}
		
		const_iterator& operator++(int)
		{
			this->operator++();
			return *this;
		}
		
		bool nodeHasValue() const
		{ 
			const Tree<V, C>* parentNode = mNodeStack.back();
			const typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			
			// no value (and currentIterator not dereferenceable!) if at end()
			if(currentIterator == parentNode->mChildren.end()) return false;

			const Tree<V, C>* currentChildNode = &((*currentIterator).second);
			return(currentChildNode->hasValue());
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
		
		std::vector< const Tree<V, C>* > mNodeStack;
		std::vector< typename mapT::const_iterator > mIteratorStack;
	};	
		
	inline const_iterator begin() const
	{
		return const_iterator(this);
	}
	
	inline const_iterator end() const
	{
		return const_iterator(this, mChildren.end());
	}

	inline void dump() const
	{
		for(auto it = begin(); it != end(); it++)
		{
			if(it.nodeHasValue())
			{		
				std::cout << ml::textUtils::spaceStr(it.getDepth()) << it.getLeafName() << " [" << it->getValue() << "]\n";
			}
			else
			{
				std::cout << ml::textUtils::spaceStr(it.getDepth()) << "/" << it.getLeafName() << "\n";
			}
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
	
	mapT mChildren;
	V mValue;
};

} // namespace ml
