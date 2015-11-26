
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_SYMBOL_MAP_H
#define _ML_SYMBOL_MAP_H

#include "MLDebug.h"
#include "MLSymbol.h"

// each MLProc-derived class creates static instances of MLSymbolMap so that 
// its parameters, inputs and outputs can be found by name.

class MLSymbolMap
{
public:	
	typedef std::map<MLSymbol, int> MLSymbolMapT;	
	typedef MLSymbolMapT::iterator MLSymbolMapIter;
	
	// indices start from one
	void addEntry(const MLSymbol name) 
	{	
		int i = (int)mMap.size() + 1;
		mMap[name] = i; 
	} 
	
	// return one-based index of parameter if found, otherwise 0.
	int getIndex(const MLSymbol paramName)
	{	
		int index = 0;

		// get named entry from parameter map
		MLSymbolMapT::const_iterator i;
		i = mMap.find(paramName);
		if (i != mMap.end()) 
		{
			index = i->second;
		}
		return index;
	} 
	
	inline int getSize() { return (int)mMap.size(); }
	
	void dump()
	{
		for (MLSymbolMapT::const_iterator i = mMap.begin(); i != mMap.end(); i++)
		{
			std::cout << "[" << (*i).first << ":" << (*i).second << "] ";
		}
	}
	
	MLSymbolMapIter begin() { return (MLSymbolMapIter)mMap.begin(); }
	MLSymbolMapIter end() { return (MLSymbolMapIter)mMap.end(); }

private:
	
	MLSymbolMapT mMap;
};

// Array class mapped by a single MLSymbolMap per template class.
// Size is static.  Get and set are by value.

// In release code, we fail silently on bad keys? what TODO
template <class arrayElement, int localStorageSize>
class SymbolMappedArray
{
public:
	SymbolMappedArray() : pMap(0) 
	{ 
		// array class must have default constructor
		mNullData = arrayElement(); 
		std::fill(mData, mData + localStorageSize, mNullData);
		mOverflowData = 0;
		mOverflowSize = 0;
	}
	
	~SymbolMappedArray() 
	{ 
		delete[] mOverflowData;
	}
	
	void setMap(const MLSymbolMap & map)
	{
		pMap = (MLSymbolMap *)&map;
	}
	
	// return element ptr by key.  If key is not found, return safe null ptr.
	arrayElement * operator[] (const MLSymbol key)
	{
		int zeroIndex, overflowIndex;
		
		arrayElement * p = &mNullData;
		if (pMap)
		{
			int idx = pMap->getIndex(key);
			if (idx)
			{
				zeroIndex = idx - 1;
				if (idx <= localStorageSize)
				{
					p = mData + zeroIndex;
				}
				else
				{
					overflowIndex = zeroIndex - localStorageSize;
					if(overflowIndex >= mOverflowSize)
					{
						// allocate 2x space needed
						if (!reallocate((overflowIndex+1)*2))
						{
							std::cout << "SymbolMappedArray::operator[]: reallocate failed!\n";
							return p;
						}
					}				
					p = mOverflowData + overflowIndex;
				}
			}
#if DEBUG			
			else
			{
				std::cout << "SymbolMappedArray::operator[]: bogus key " << key << "!\n";
				std::cout << "    map: ";
				pMap->dump();
				std::cout << "\n";
			}
#endif
		}
		else
		{
			std::cout << "SymbolMappedArray::operator[]: aiieee, no map!\n";
		}
		return p; 
	}
                
    arrayElement* getNullElement() const { return &mNullData; }
	
private:

	arrayElement* reallocate(int newSize)
	{
		arrayElement* pTemp;
		pTemp = new arrayElement[newSize];
		if (pTemp)
		{
			std::copy(mOverflowData, mOverflowData + mOverflowSize, pTemp);
			std::fill(pTemp + mOverflowSize, pTemp + newSize, mNullData);
			delete[] mOverflowData;
			mOverflowData = pTemp;
			mOverflowSize = newSize;
		}
		return pTemp;
	}

	// TODO investigate problem with non-local storage !?
	arrayElement mData[localStorageSize];
	arrayElement* mOverflowData;
	int mOverflowSize;
	
	MLSymbolMap* pMap;
	
	// one extra element per class
	static arrayElement mNullData;
};

template <class arrayElement, int size>
	 arrayElement SymbolMappedArray<arrayElement, size>::mNullData;


#endif	// _ML_SYMBOL_MAP_H