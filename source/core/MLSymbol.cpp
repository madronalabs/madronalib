
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
		// TODO better protection on delete
		// can this be avoided with more explicit setup / shutdown
		// (RAII in main() )
		// std::unique_lock<std::mutex> lock(mMutex);

	}

	// clear all symbols from the table.
	void SymbolTable::clear()
	{
		mSize = 0;		
		// std::unique_lock<std::mutex> lock(mMutex);
		
		mSymbolTextsByID.clear();
		mSymbolTextsByID.reserve(kDefaultSymbolTableSize);

#if OLD_TABLE
//		mHashTable.clear();
//		mHashTable.resize(kHashTableSize);	
		for(int i=0; i<kHashTableSize; ++i)
		{
			clearEntry(mHashTable[i]);
		}		
#else
		for(int i=0; i<kHashTableSize; ++i)
		{
			mHashTable[i] = 0; // clear entry
		}

#endif		
		// add null entry - why?
		addEntry(HashedCharArray());
	}

	
	// add an entry to the table. The entry must not already exist in the table.
	// this must be the only way of modifying the symbol table.
	int SymbolTable::addEntry(const HashedCharArray& hsl)
	{		
		mSymbolTextsByID.emplace_back(TextFragment(hsl.pChars, static_cast<int>(hsl.len)));
		
		int newID = mSize++;
		
#if OLD_TABLE	
		
		// std::cout << "adding " << newID << " at " << hsl.hash << "\n";
		mHashTable[hsl.hash].mIDVector.emplace_back(newID);
		
#else
		
		// look for empty bin in hash table
		int idx = hsl.hash;
		
		// MLTEST
		int tryCount = 0;
		
		while(1)
		{
			tryCount++;
						
			int expected = 0;
			bool exchanged = false;
			
			// test
			// std::atomic<int> t;
			
			exchanged = mHashTable[idx].compare_exchange_strong(expected, newID);//, std::memory_order_relaxed);
			
			int prevID = expected;
			
			// we want to end on two conditions: either a successful swap with a free entry, or 
			// a failure because the table is full.
			
			if(exchanged) 
			{
				// std::cout << prevID << " -> " << newID << "\n"; 
			//	if(tryCount > 1)
				{
					std::cout << "success: " << prevID << " -> " << newID << " at " << idx <<  " tryCount: " << tryCount << "\n";
				}
				break;	
			}
			else
			{
				std::cout << "fail: " << prevID << " was home at " << idx << "\n";
			}
			idx++;
			idx &= kHashTableMask;
			
			// need to fail here if full
		}
		// set empty bin to ID of new entry
		// mHashTable[idx] = newID;
#endif		
		return newID;
	}

#if OLD_TABLE	
	
	int SymbolTable::getSymbolID(const HashedCharArray& hsl)
	{
		int r = 0;
		
		// get the vector of symbol IDs matching this hash. It probably has one entry but may have more. 
		const std::vector<int>& bin = mHashTable[hsl.hash].mIDVector;
		{
			bool found = false;
			
			std::unique_lock<std::mutex> lock(mHashTable[hsl.hash].mMutex);			

			 for(int ID : bin)
			 {
				// there should be few collisions, so probably the first ID in the hash bin
				// will be the symbol we are looking for. Unfortunately to test for equality we may have to 
				// compare the entire string.	
			 
				TextFragment* binFragment = &mSymbolTextsByID[ID];
				if(compareSizedCharArrays(binFragment->getText(), binFragment->lengthInBytes(), hsl.pChars, hsl.len))
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

	
#else
	
	int SymbolTable::getSymbolID(const HashedCharArray& hsl)
	{
		int r = 0;
		
		// get the vector of symbol IDs matching this hash. It probably has one entry but may have more. 
		int idx = hsl.hash;
	//	{
	////		std::unique_lock<std::mutex> lock(mMutex);			
			
			// advance while not found and not free (0)
			while(1)
			{				
				int ID = mHashTable[idx].load(std::memory_order_seq_cst);
				
				// int ID = mHashTable[idx];
				
				//if(0 == ID) break;
				
				if(ID != 0)
				{
					TextFragment* binFragment = &mSymbolTextsByID[ID];
					if(compareSizedCharArrays(binFragment->getText(), binFragment->lengthInBytes(), hsl.pChars, hsl.len))
					{
						return ID;
					}
				}
				else
				{
					break;
				}
				
				idx++;
				idx &= kHashTableMask;

				// need to fail here if full
			}


			r = addEntry(hsl);
			
	//	}
		return r;
	}
	
#endif
	
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

#if OLD_TABLE
		for(auto& tableEntry : mHashTable)
		{
			auto idVec = tableEntry.mIDVector;
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
#else
		
		for(int i=0; i<kHashTableSize; ++i)
		{
			int id = mHashTable[i];
			if(id)
			{
				std::cout << i << ": " << id << " " << getSymbolTextByID(id) << " \n";
			}
		}
		
#endif		
	}

	
	int SymbolTable::audit()
	{
		// MLTEST
		return true;

		int i=0;
		int i2 = 0;
		bool OK = true;
		size_t size = mSymbolTextsByID.size();
	 
		for(i=0; i<size; ++i)
		{
			const TextFragment& sym = getSymbolTextByID(i);
			const char* symChars = sym.getText();
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
