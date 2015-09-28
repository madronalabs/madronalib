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
	MLResourceMap<T>() : mValue() {}
	MLResourceMap(const T& v) { mValue = v; }
	~MLResourceMap() {}
	
	void clear() { mChildren.clear(); }
	const T& getValue() const { return mValue; }
	void setValue(const T& v) { mValue = v; }
	int getNumChildren() const { return mChildren.size(); }

	// TODO until we have our own proper iterator, this hack allows recursion from outside
	const MLResourceMap<T>& getChild(int n) const 
	{ 
		typename mapT::const_iterator it = mChildren.begin();
		std::advance(it, n);
		return (*it).second; 
	}

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
	void addValue (const std::string& pathStr, const T& v)
	{
		addNode(pathStr)->setValue(v);
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
	
	// build a linear index to the leaf nodes of the tree.
	//This is wrong because of empty directories!
	//Just make our own Iterator.
	void buildLeafIndex(std::vector<T>& index) const
	{
		typename MLResourceMap<T>::mapT::const_iterator it;
		for(it = mChildren.begin(); it != mChildren.end(); ++it)
		{
			const MLResourceMap& node = it->second;
			if(node.getNumChildren())
			{
				node.buildLeafIndex(index);
			}
			else
			{
				// add leaf nodes to index
				
				// TODO leaf only iterator!
				index.push_back(node.getValue());
			}
		}
	}
	
	class const_iterator
	{
	public:
		const_iterator()  {}
		const_iterator(const MLResourceMap<T>* p)  
		{
			std::cout << "new iterator: \n"; 
			std::cout << "    chillins:\n";
			const MLResourceMap<T>* test = p;			
			for(auto mapIter = test->mChildren.begin(); mapIter != test->mChildren.end(); ++mapIter)
			{
				std::cout << "    " << mapIter->first << "\n";
			}
			
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
			bool eq = (mIteratorStack.back() == b.mIteratorStack.back()); 
			std::cout << "equals? " << eq << "\n";
			return (mIteratorStack.back() == b.mIteratorStack.back()); 
		}
		
		bool operator!=(const MLResourceMap<T>::const_iterator& b) const 
		{ 
			return !(*this == b); 
		}
				
		const MLResourceMap<T>& operator*() const 
		{ 
			return *mIteratorStack.back(); 
		}
		
		const MLResourceMap<T>* operator->() const 
		{ 
			return mIteratorStack.back(); 
		}
	
		const MLResourceMap<T>::const_iterator& operator++()
		{			
			// MLTEST
			const MLResourceMap<T>* parentNode = mNodeStack.back();
			typename mapT::const_iterator& currentIterator = mIteratorStack.back();
			const MLResourceMap<T>* currentChildNode = &((*currentIterator).second);			
			
			//const MLResourceMap<T>* thisNode = &((*currentIterator).second);		
			
			std::cout << "iterator++: children " << parentNode->getNumChildren() << "\n";
			
			for(auto mapIter = parentNode->mChildren.begin(); mapIter != parentNode->mChildren.end(); ++mapIter)
			{
				std::cout << "    " << mapIter->first << "\n";
			}
			
			if(currentIterator == parentNode->mChildren.end())
			{
				std::cout << "UP\n";
				mNodeStack.pop_back();
				mIteratorStack.pop_back();
				mIteratorStack.back()++;			
			}			
			else if(currentChildNode->getNumChildren())
			{
				std::cout << "DOWN\n";
				mNodeStack.push_back(currentChildNode);
				mIteratorStack.push_back(currentChildNode->mChildren.begin());
			}
			else 
			{
				std::cout << "ACROSS\n";
				currentIterator++;			
			}

			
			// if(currentIterator != parentNode->mChildren.end())
			 
			return *this;
		}
		
		MLResourceMap<T>::const_iterator& operator++(int)
		{
			this->operator++();
			return *this;
		}
		
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
			pNode = &(pNode->mChildren[sym]);
		}
		
		return pNode;
	}
	
	mapT mChildren;
	T mValue;
};


#endif /* defined(__ResourceMap__) */
