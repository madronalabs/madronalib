
// MLSymbol.h
// ----------

// ml::Symbol is designed to be an efficient key in STL containers such as map and
// unordered_map, that is quick to convert to and from a unique UTF-8 string.  
//
// requirements
// ---------
//
// Symbols are immutable.
// The value of a Symbol must remain valid even after more MLSymbols are created.  
// This allows MLSymbols to function as keys in any kind of data structure.
//
// Accessing an Symbol must not cause any heap to be allocated if the symbol already exists. 
// This allows use in DSP code, assuming that the signal graph or whatever has already been parsed.

#pragma once

#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <algorithm>

#include "MLLocks.h"
#include "MLTextFragment.h"

namespace ml {
	
	static const int kMLMaxSymbolLength = 56;
	static const int kMLMaxNumberLength = 8;
	
	const int kHashTableBits = 12;
	const int kHashTableSize = (1 << kHashTableBits);
	const int kHashTableMask = kHashTableSize - 1;
	
	// symbols are allocated in chunks of this size when needed. 
	const int kTableChunkSize = 1024;
	
	// very simple hash function from Kernighan & Ritchie. 
	// Constexpr version for hashing strings known at compile time.
	template <size_t N>
	constexpr uint32_t krHash2(const char * str) 
	{
		return (N > 1) ? ((krHash2<N - 1>(str + 1))*31u + *str) : 0;
	}
	
	template <>
	constexpr uint32_t krHash2<size_t(0)>(const char * str) 
	{
		return 0;
	}
	
	template <size_t N>
	constexpr uint32_t krHash1(const char * str) 
	{
		return krHash2<N>(str) & kHashTableMask;
	}
	
	// non-recursive hash producing equivalent results to krHash1. 
	// Non-constexpr version for hashing strings known only at runtime.
	inline uint32_t krHash0(const char * str, const size_t len) 
	{
		int i = len;
		uint32_t accum = 0;
		while(i > 0)
		{
			i--;
			accum *= 31u;
			accum += str[i];
		}
		return accum & kHashTableMask;
	}
	
	class HashedCharArray
	{
	public:	
		// template ctor from string literals allows hashing for code like Proc::setParam("foo") to be done at compile time.
		template<size_t N>
		constexpr HashedCharArray(const char (&sym)[N]) : len(N), hash(krHash1<N>(sym)), pSym(sym) { }
		
		// this non-constexpr ctor counts the string length at runtime.
		HashedCharArray(const char* pC) : len(strlen(pC)), hash(krHash0(pC, len)), pSym(pC) { }
		
		// this non-constexpr ctor takes a string length parameter at runtime.
		HashedCharArray(const char* pC, int lengthBytes) : len(lengthBytes), hash(krHash0(pC, len)), pSym(pC) { }
		
		// default, null ctor
		HashedCharArray() : len(0), hash(0), pSym(nullptr) { }
		
		const size_t len;
		const int32_t hash;
		const char* pSym;
	};
	
	class SymbolTable
	{
		friend class Symbol;
	public:
		SymbolTable();
		~SymbolTable();
		void clear();
		size_t getSize() { return mSymbolTextsByID.size(); }	
		void dump(void);
		int audit(void);
		
	protected:
		
		// look up a symbol by name and return its ID. Used in Symbol constructors.
		// if the symbol already exists, this routine must not allocate any heap memory.
		int getSymbolID(const HashedCharArray& hsl);
		int getSymbolID(const char * sym);
		int getSymbolID(const char * sym, int lengthBytes);
		
		const TextFragment& getSymbolTextByID(int symID);
		int addEntry(const HashedCharArray& hsl);
		
	private:
		// ensure symbol table integrity with simple SpinLock.
		MLSpinLock mLock;
		
		// vector of symbols in ID/creation order
		std::vector< TextFragment > mSymbolTextsByID;	
		
		// hash table containing indexes to strings
		std::vector< std::vector<int> > mHashTable;
		
		int mSize;
	};
	
	inline SymbolTable& theSymbolTable()
	{
		static std::unique_ptr<SymbolTable> t (new SymbolTable());
		return *t;
	}
	
	// ----------------------------------------------------------------
#pragma mark Symbol
	
	class Symbol
	{
		friend std::ostream& operator<< (std::ostream& out, const Symbol r);
		
	public:
		Symbol() : id(0) {}
		Symbol(const HashedCharArray& hsl) : id( theSymbolTable().getSymbolID(hsl) ) { }		
		Symbol(const char* pC) : id( theSymbolTable().getSymbolID(pC) ) { }
		Symbol(const char* pC, int lengthBytes) : id( theSymbolTable().getSymbolID(pC, lengthBytes) ) { }
		Symbol(TextFragment frag) : id( theSymbolTable().getSymbolID(frag.text) ) { }
		
		inline bool operator< (const Symbol b) const
		{
			return(id < b.id);
		}
		
		inline bool operator== (const Symbol b) const
		{
			return (id == b.id);
		}	
		
		inline bool operator!= (const Symbol b) const
		{
			return (id != b.id);
		}	
		
		explicit operator bool() const { return id != 0; }
		
		// search hash table for our id to find our hash.
		// for testing only! 
		inline int getHash() const 
		{ 
			int hash = 0;
			for(auto idVec : theSymbolTable().mHashTable)
			{
				size_t idVecLen = idVec.size();
				if(idVecLen > 0)
				{
					for(auto vid : idVec)
					{
						if(vid == id)
						{
							return hash;
						}
					}
				}
				hash++;
			}		
			
			return 0; 
		}
		
		// return the symbol's TextFragment in the table.
		// in order to show the strings in XCode's debugger, instead of the unhelpful id,
		// edit the summary format for Symbol within XCode to {$VAR.getTextFragment().text}:s
		inline const TextFragment getTextFragment() const
		{
			return theSymbolTable().getSymbolTextByID(id);
		}
		
		// the ID equals the order in which the symbol was created.
		// 2^31 unique symbols are possible. There is no checking for overflow.
		const int id;
	};
	
	inline Symbol operator+(Symbol f1, Symbol f2)
	{
		return Symbol(f1.getTextFragment() + f2.getTextFragment());
	}
}	// namespace ml

// hashing function for ml::Symbol use in unordered STL containers. simply return the ID,
// which gives each Symbol a unique hash.
namespace std 
{
	template<>
	struct hash<ml::Symbol>
	{
		std::size_t operator()(const ml::Symbol& s) const
		{
			return s.id;
		}
	};
}
