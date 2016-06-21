
// ml::Symbol.cpp
// ----------
// Madrona Labs C++ framework for DSP applications.
// Copyright (c) 2015 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSymbol.h"

namespace ml {

	#pragma mark SymbolTable

	SymbolTable::SymbolTable()
	{
		clear();
	}

	SymbolTable::~SymbolTable()
	{
	}

	// clear all symbols from the table.
	void SymbolTable::clear()
	{
		mSize = 0;
		MLScopedLock lock(mLock);
		mSymbolTextsByID.clear();
		
		mHashTable.clear();
		mHashTable.resize(kHashTableSize);	
		addEntry(HashedCharArray());
	}

	// add an entry to the table. The entry must not already exist in the table.
	// this must be the only way of modifying the symbol table.
	int SymbolTable::addEntry(const HashedCharArray& hsl)
	{
		mSymbolTextsByID.emplace_back(TextFragment(hsl.pSym, hsl.len));
		size_t newID = mSize++;
		mHashTable[hsl.hash].push_back(newID);			
		return newID;
	}

	int SymbolTable::getSymbolID(const HashedCharArray& hsl)
	{
		int r = 0;
		bool found = false;
		const std::vector<int>& bin = mHashTable[hsl.hash];
		{
			MLScopedLock lock(mLock);
			for(int ID : bin)
			{
				// there should be few collisions, so probably the first ID in the hash bin
				// will be the symbol we are looking for. Unfortunately to test for equality we have to 
				// compare the entire string.	
				if(compareTextFragmentToChars(mSymbolTextsByID[ID], hsl.pSym))
				{
					r = ID;
					found = true;
					break;
				}
			}
			
			if(!found)
			{	
				r = addEntry(hsl);
			}
		}
		return r;
	}

	int SymbolTable::getSymbolID(const char * sym)
	{
		return getSymbolID(HashedCharArray(sym));
	}
	
	int SymbolTable::getSymbolID(const char * sym, int lengthBytes)
	{
		return getSymbolID(HashedCharArray(sym, lengthBytes));
	}
	
	const TextFragment& SymbolTable::getSymbolTextByID(int symID)
	{
		return mSymbolTextsByID[symID];
	}

	void SymbolTable::dump()
	{
		std::cout << "---------------------------------------------------------\n";
		std::cout << mSymbolTextsByID.size() << " symbols:\n";
			
		// print symbols in order of creation. 
		for(int i=0; i<mSymbolTextsByID.size(); ++i)
		{
			const TextFragment& sym = mSymbolTextsByID[i];
			std::cout << "    ID " << i << " = " << sym << "\n";
		}	
		// print nonzero entries in hash table
		int hash = 0;
		for(auto idVec : mHashTable)
		{
			size_t idVecLen = idVec.size();
			if(idVecLen > 0)
			{
				std::cout << "#" << hash << " ";
				for(auto id : idVec)
				{
					std::cout << id << " " << getSymbolTextByID(id) << " ";
				}

				std::cout << "\n";
			}
			hash++;
		}
	}

	int SymbolTable::audit()
	{
		int i=0;
		int i2 = 0;
		bool OK = true;
		size_t size = mSymbolTextsByID.size();
	 
		for(i=0; i<size; ++i)
		{
			const TextFragment& sym = getSymbolTextByID(i);
			const char* symChars = sym.text;
			Symbol symB(symChars);

			i2 = symB.getID();
			if (i != i2)
			{
				OK = false;
				break;
			}
			if (i2 > size)
			{
				OK = false;
				break;
			}
		}
		if (!OK)
		{
			const TextFragment& s = getSymbolTextByID(i);
			std::cout << "SymbolTable: error in symbol table, line " << i << ":\n";
			std::cout << "    ID " << i << " = " << s << ", ID B = " << i2 << "\n";
		}
		return OK;
	}
	
	std::ostream& operator<< (std::ostream& out, const Symbol r)
	{
		out << r.getTextFragment();
		return out;
	}	
} // namespace ml
