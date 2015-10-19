//
//  MLResourceMap.h
//  madronaib
//
//  Created by Randy Jones on 9/22/15.
//
//  A map to a hierarchical container of resources, such as a directory structure.
//
//

#ifndef __ResourceMap__
#define __ResourceMap__

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>

#include "MLStringUtils.h" 

// A resource map using a key class K, value class V, and optional comparator class C.
// The value class must have a default constructor V() returning a safe null object.
// Note that this makes MLResourceMap<..., int> weird to use, because 0 indicates
// a null value. However, we are typically interested in more complex value types.
//

template < class K, class V, class C = ml::stringUtils::caseInsensitiveCompare<K> >
class MLResourceMap
{
public:
	typedef std::map< K, MLResourceMap<K, V, C>, C > mapT;

	MLResourceMap<K, V, C>() : mChildren(), mValue() { }
	~MLResourceMap<K, V, C>() {}
	
	void clear() { mChildren.clear(); }
	const V& getValue() const { return mValue; }
	void setValue(const V& v) { mValue = v; }
	bool hasValue() const {  return mValue != V(); } 
	bool isLeaf() const { return mChildren.size() == 0; }
	
	// find a value by its path.	
	// if the path exists, returns the value in the tree.
	// else, return a null object of our value type V.
	V findValue(const std::string& path)
	{
		MLResourceMap<K, V, C>* pNode = findNode(path);
		if(pNode)
		{
			return pNode->getValue();
		}
		else
		{
			return V();
		}
	}

	std::vector< K > parsePath(const std::string& pathStr)
	{
		std::vector< K > path;
		std::string workingStr = pathStr;
		int segmentLength;
		do
		{
			// found a segment
			segmentLength = workingStr.find_first_of("/");
			if(segmentLength)
			{
				// iterate
				path.push_back(workingStr.substr(0, segmentLength));
				workingStr = workingStr.substr(segmentLength + 1, workingStr.length() - segmentLength);
			}
			else
			{
				// leading slash or null segment
				workingStr = workingStr.substr(1, workingStr.length());
			}
		}
		while(segmentLength != std::string::npos);
		return path;
	}

	// add a map node at the specified path, and any parent nodes necessary in order to put it there.
	// If a node already exists at the path, return the existing node,
	// else return a pointer to the new node.
	MLResourceMap<K, V, C>* addNode(const std::string& pathStr)
	{
		MLResourceMap<K, V, C>* pNode = this;
		
		// using an intermediate std::vector path is convenient but inefficient,
		// we can just walk pathStr directly if more efficiency is wanted here
		typename std::vector< K > path = parsePath(pathStr);
		typename std::vector< K >::const_iterator it;
		int pathDepthFound = 0;
		
		// walk the path as long as branches are found in the map
		for(K key : path)
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
		for(it = path.begin() + pathDepthFound; it != path.end(); ++it)
		{
			K key = *it;
			
			// [] operator crates the new node
			pNode = &(pNode->mChildren[key]);
		}
		
		return pNode;
	}
	
	// TODO use MLSymbol vector paths
	MLResourceMap<K, V, C>* addValue (const std::string& pathStr, const V& val)
	{
		MLResourceMap<K, V, C>* newNode = addNode(pathStr);
		newNode->setValue(val);
		return newNode;
	}
	
	// TODO this iterator does not work with STL algorithms in general, only for simple begin(), end() loops.
	// add the other methods needed.
	
	friend class const_iterator;
	class const_iterator
	{
	public:
		const_iterator(const MLResourceMap<K, V, C>* p)  
		{
			mNodeStack.push_back(p); 
			mIteratorStack.push_back(p->mChildren.begin());
		}
		
		const_iterator(const MLResourceMap<K, V, C>* p, const typename mapT::const_iterator subIter)
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
				
		const MLResourceMap<K, V, C>& operator*() const 
		{ 
			return ((*mIteratorStack.back()).second); 
		}
		
		const MLResourceMap<K, V, C>* operator->() const 
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
				const MLResourceMap<K, V, C>* currentChildNode = &((*currentIterator).second);
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
			const MLResourceMap<K, V, C>* parentNode = mNodeStack.back();
			const typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			
			// no value (and currentIterator not dereferenceable!) if at end()
			if(currentIterator == parentNode->mChildren.end()) return false;

			const MLResourceMap<K, V, C>* currentChildNode = &((*currentIterator).second);
			return(currentChildNode->hasValue());
		}
		
		bool atEndOfMap() const
		{
			const MLResourceMap<K, V, C>* parentNode = mNodeStack.back();
			const typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			return(currentIterator == parentNode->mChildren.end());
		}
		
		K getLeafName() const
		{
			const MLResourceMap<K, V, C>* parentNode = mNodeStack.back();
			const typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			
			// no value (and currentIterator not dereferenceable!) if at end()
			if(currentIterator == parentNode->mChildren.end()) return K();
			
			return (*currentIterator).first;
		}

		int getDepth() { return mNodeStack.size() - 1; }
		
		std::vector< const MLResourceMap<K, V, C>* > mNodeStack;
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

private:

	// find a tree node at the specified path. 
	// if successful, return a pointer to the node. If unsuccessful, return nullptr.
	MLResourceMap<K, V, C>* findNode(const std::string& pathStr)
	{
		MLResourceMap<K, V, C>* pNode = this;
		std::vector< K > path = parsePath(pathStr);		
		for(K key : path)
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


#endif /* defined(__ResourceMap__) */
