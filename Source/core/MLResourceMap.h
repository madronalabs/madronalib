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
	
	// build a linear index to the leaf nodes of the tree.
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
				index.push_back(node.getValue());
			}
		}
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
