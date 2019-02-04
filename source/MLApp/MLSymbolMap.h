
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_SYMBOL_MAP_H
#define _ML_SYMBOL_MAP_H

#include "MLDebug.h"
#include "MLSymbol.h"
#include <unordered_map>

// each MLProc-derived class creates static instances of MLSymbolMap so that
// its parameters, inputs and outputs can be found by name.

class MLSymbolMap
{
public:
    //	typedef std::vector<ml::Symbol> MLSymbolMapT;
    //	typedef MLSymbolMapT::iterator MLSymbolMapIter;
    
    // indices start from one
    void addEntry(const ml::Symbol name)
    {
        //int i = (int)mMap.size() + 1;
        //mMap[name] = i;
        mSymbols.emplace_back(name);
    }
    
    // return one-based index of symbol if found, otherwise 0.
    int getIndex(const ml::Symbol sym)
    {
        int index = 0;
        int v = mSymbols.size();
        for(int i=0; i<v; ++i)
        {
            if(mSymbols[i] == sym)
            {
                index = i+1;
                break;
            }
        }
        return index;
    }
    
    // get symbol at one-based index.
    ml::Symbol getSymbolAtIndex(int i)
    {
        return(i <= mSymbols.size()) ? mSymbols[i - 1] : 0;
    }
    
    inline int getSize() { return (int)mSymbols.size(); }
    
    void dump()
    {
        int v = mSymbols.size();
        for(int i=0; i<v; ++i)
        {
            std::cout << "[" << mSymbols[i] << "] ";
        }
    }
    
private:
    
    std::vector<ml::Symbol> mSymbols;
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
    arrayElement * operator[] (const ml::Symbol key)
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
                            //std::cout << "SymbolMappedArray::operator[]: reallocate failed!\n";
                            return p;
                        }
                    }
                    p = mOverflowData + overflowIndex;
                }
            }
#if DEBUG
            else
            {
                //std::cout << "SymbolMappedArray::operator[]: bogus key " << key << "!\n";
                //std::cout << "    map: ";
                pMap->dump();
                //std::cout << "\n";
            }
#endif
        }
        else
        {
            //std::cout << "SymbolMappedArray::operator[]: aiieee, no map!\n";
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
