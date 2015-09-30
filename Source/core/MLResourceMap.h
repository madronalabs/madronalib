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

#include "MLSymbol.h"
#include "MLStringCompare.h" 
#include "MLStringUtils.h" 

template < class T >
class MLResourceMap
{
public:
	typedef std::map<MLSymbol, MLResourceMap<T> > mapT;
	typedef std::map<MLSymbol, const MLResourceMap<T> > constMapT;
	
	// our value class must have a default constructor returning a safe null object.
	MLResourceMap<T>() : mValue(), mIsLeaf(false) {}
	MLResourceMap(const T& v) : mIsLeaf(false) { mValue = v; }
	~MLResourceMap() {}
	
	void clear() { mChildren.clear(); }
	const T& getValue() const { return mValue; }
	void setValue(const T& v) { mValue = v; }
	
	bool isLeaf() const { return mIsLeaf; }
	void markAsLeaf(bool b) { mIsLeaf = b; }
	
//	int getNumChildren() const { return mChildren.size(); }

	// find a value by its path.	
	// if the path exists, returns the value in the tree.
	// else, return a null object of our value type T.
	T findValue(const std::string& path)
	{
		MLResourceMap<T>* pNode = findNode(path);
		if(pNode)
		{
			return pNode->getValue();
		}
		else
		{
			return T();
		}
	}
	
	// TODO also use MLSymbol vector paths
	MLResourceMap<T>* addValue (const std::string& pathStr, const T& v, bool isLeaf = false)
	{
		MLResourceMap<T>* newNode = addNode(pathStr);
		newNode->setValue(v);
		newNode->markAsLeaf(isLeaf);
		return newNode;
	}
	
	void dump(int level = 0) const
	{
		typename MLResourceMap<T>::mapT::const_iterator it;
		
		for(it = mChildren.begin(); it != mChildren.end(); ++it)
		{
			MLSymbol p = it->first;
			const MLResourceMap<T> n = it->second;		
			std::cout << level << ": " << MLStringUtils::spaceStr(level) << p << ":" << n.mValue << "\n";
			n.dump(level + 1);
		}
	}
	
	friend class const_iterator;
	class const_iterator
	{
		static const MLResourceMap<T> mNullValue;
	public:
		const_iterator()  {}
		const_iterator(const MLResourceMap<T>* p)  
		{
			mNodeStack.push_back(p); 
			mIteratorStack.push_back(p->mChildren.begin());
		}
		
		const_iterator(const MLResourceMap<T>* p, const typename mapT::const_iterator subIter)
		{
			mNodeStack.push_back(p); 
			mIteratorStack.push_back(subIter);
		}
		~const_iterator() {}
		
		bool operator==(const MLResourceMap<T>::const_iterator& b) const 
		{ 
			return (mIteratorStack.back() == b.mIteratorStack.back()); 
		}
		
		bool operator!=(const MLResourceMap<T>::const_iterator& b) const 
		{ 
			return !(*this == b); 
		}
				
		const MLResourceMap<T>& operator*() const 
		{ 
			return ((*mIteratorStack.back()).second); 
		}
		
		const MLResourceMap<T>* operator->() const 
		{ 
			return &((*mIteratorStack.back()).second); 
		}
		
		const MLResourceMap<T>::const_iterator& operator++()
		{			
			typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			const MLResourceMap<T>* currentChildNode = &((*currentIterator).second);			
			
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
			else if(!currentChildNode->isLeaf())
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
			 
			return *this;
		}
		
		MLResourceMap<T>::const_iterator& operator++(int)
		{
			this->operator++();
			return *this;
		}
		
		bool atLeaf() const
		{ 
			const MLResourceMap<T>* parentNode = mNodeStack.back();
			const typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			const MLResourceMap<T>* currentChildNode = &((*currentIterator).second);	
			
			// not a leaf if at end()
			if(currentIterator == parentNode->mChildren.end()) return false;

			// return(currentChildNode->getNumChildren() == 0);
			// TODO remove mLeaf when directories are stored implicitly as paths.
			return(currentChildNode->isLeaf());
		}
		
		bool atEndOfMap() const
		{ 
			const MLResourceMap<T>* parentNode = mNodeStack.back();
			const typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			
			return(currentIterator == parentNode->mChildren.end());
		}
		
		int getDepth() { return mNodeStack.size() - 1; }
		
		std::vector< const MLResourceMap<T>* > mNodeStack;
		std::vector< typename mapT::const_iterator > mIteratorStack;
	};	
		
	inline MLResourceMap<T>::const_iterator begin() const
	{
		return const_iterator(this);
	}
	
	inline MLResourceMap<T>::const_iterator end() const
	{
		return const_iterator(this, mChildren.end());
	}

private:

	// find a tree node at the specified path. 
	// if successful, return a pointer to the node. If unsuccessful, return nullptr.
	MLResourceMap<T>* findNode(const std::string& pathStr)
	{
		MLResourceMap<T>* pNode = this;
		std::vector< MLSymbol > path = MLStringUtils::parsePath(pathStr);		
		for(MLSymbol sym : path)
		{
			if(pNode->mChildren.find(sym) != pNode->mChildren.end())
			{
				pNode = &(pNode->mChildren[sym]);
			}
			else
			{
				pNode = nullptr;
				break;
			}
		}
		return pNode;
	}
	
	// add a tree node at the specified path, and any parent nodes necessary in order to put it there.
	// If a tree node already exists at the path, return the existing node,
	// else return a pointer to the new node.
	MLResourceMap<T>* addNode(const std::string& pathStr)
	{
		MLResourceMap<T>* pNode = this;
		std::vector< MLSymbol > path = MLStringUtils::parsePath(pathStr);
		std::vector< MLSymbol >::const_iterator it;
		int pathDepthFound = 0;
		
		// walk the path as long as branches are found in the map
		for(MLSymbol sym : path)
		{
			if(pNode->mChildren.find(sym) != pNode->mChildren.end())
			{
				pNode = &(pNode->mChildren[sym]);
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
			MLSymbol sym = *it;
			
			// [] operator crates the new node
			pNode = &(pNode->mChildren[sym]);
		}
		
		return pNode;
	}
	
	mapT mChildren;
	T mValue;
	bool mIsLeaf;	// TODO remove when directories are stored implicitly as paths.
};


#endif /* defined(__ResourceMap__) */
