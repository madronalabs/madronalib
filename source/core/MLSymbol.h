
// MLSymbol.h
// ----------

// MLSymbol is designed to be an efficient key in STL containers such as map and
// unordered_map, that is quick to convert to and from a unique string.  
//
// requirements
// ---------
//
// Symbols are immutable.
// The value of an MLSymbol must remain valid even after more MLSymbols are created.  
// This allows MLSymbols to function as correct keys.
//
// Accessing an MLSymbol must not cause any heap to be allocated if the symbol already exists. 
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

// With USE_ALPHA_SORT on, a std::map<MLSymbol, ...> will be in alphabetical order.
// With it off, the symbols will sort into the order they were created, and symbol creation 
// as well as map lookups will be significantly faster. 
#define USE_ALPHA_SORT	0

static const int kMLMaxSymbolLength = 56;
static const int kMLMaxNumberLength = 8;

const int kHashTableBits = 12;
const int kHashTableSize = (1 << kHashTableBits);
const int kHashTableMask = kHashTableSize - 1;

// symbols are allocated in chunks of this size when needed. 
const int kTableChunkSize = 1024;

// very simple hash function from Kernighan & Ritchie. Constexpr version.
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

// non-constexpr non-recursive version.
inline uint32_t krHash0(const char * str, const size_t len) 
{
	if(!len) return 0;
	size_t i = len - 1;
	uint32_t accum = str[i];
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
	constexpr HashedCharArray(const char (&sym)[N]) : hash(krHash1<N>(sym)), pSym(sym), len(N) { }
	
	// this non-constexpr ctor takes an external length parameter, so that we can have an immutable object
	// (all data const) and not have to call strlen twice.
	HashedCharArray(const char* pC, const size_t extLen) : hash(krHash0(pC, extLen)), pSym(pC), len(extLen) {}
	
	const int32_t hash;
	const char* pSym;
	const size_t len;
};

class MLSymbolTable
{
friend class MLSymbol;
public:
	MLSymbolTable();
	~MLSymbolTable();
	void clear();
	size_t getSize() { return mSymbolsByID.size(); }	
	void dump(void);
	int audit(void);
	
protected:
	// look up a symbol by name and return its ID. Used in MLSymbol constructors.
	// if the symbol already exists, this routine must not allocate any heap memory.
	int getSymbolID(const char * sym);
	int getSymbolID(const HashedCharArray& hsl);
	
	const TextFragment& getSymbolByID(int symID);
	int addEntry(const char * sym, uint32_t hash);
	
#if USE_ALPHA_SORT	
	int getSymbolAlphaOrder(const int symID);
#endif
	
private:
	// ensure symbol table integrity with simple SpinLock.
	MLSpinLock mLock;
		
	// vector of symbols in ID/creation order
	std::vector< TextFragment > mSymbolsByID;	
	
	// hash table containing indexes to strings
	std::vector< std::vector<int> > mHashTable;
	
#if USE_ALPHA_SORT	
	// vector of alphabetically sorted indexes into symbol vector, in ID order
	std::vector<int> mAlphaOrderByID;	
	
	// std::set is used for sorting.
	std::set< TextFragment, MLStringCompareFn > mSymbolsByAlphaOrder;
#endif

};

inline MLSymbolTable& theSymbolTable()
{
	static std::unique_ptr<MLSymbolTable> t (new MLSymbolTable());
	return *t;
}

// ----------------------------------------------------------------
#pragma mark MLSymbol

class MLSymbol
{
	friend std::ostream& operator<< (std::ostream& out, const MLSymbol r);
	
public:
	MLSymbol() : id(0) {}

	MLSymbol(const HashedCharArray& hsl) : id( theSymbolTable().getSymbolID(hsl) ) { }
	
	MLSymbol(const char* pC) : id( theSymbolTable().getSymbolID(pC) ) { }
	
	inline bool operator< (const MLSymbol b) const
	{
#if USE_ALPHA_SORT			
		return (theSymbolTable().getSymbolAlphaOrder(id) < theSymbolTable().getSymbolAlphaOrder(b.id));
#else
		return(id < b.id);
#endif
	}
	
	inline bool operator== (const MLSymbol b) const
	{
		return (id == b.id);
	}	
	
	inline bool operator!= (const MLSymbol b) const
	{
		return (id != b.id);
	}	
	
	explicit operator bool() const { return id != 0; }
	
	// MLTEST
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
	
	// in order to show the strings in XCode's debugger, instead of the unhelpful id,
	// edit the summary format for MLSymbol within XCode to {$VAR.getString()}:s
	const TextFragment& getTextFragment() const;
	
//	bool beginsWith (const MLSymbol b) const;
//	bool endsWith (const MLSymbol b) const;
	bool hasWildCard() const;
	
	int getFinalNumber() const;	
	int compare(const char *str) const;
	
	// TODO make free functions
	MLSymbol append(const TextFragment& b) const;
	MLSymbol withWildCardNumber(int n) const;
	MLSymbol withFinalNumber(int n) const;
	MLSymbol withoutFinalNumber() const;
	
	// the ID equals the order in which the symbol was created.
	// 2^31 unique symbols are possible. There is no checking for overflow.
	const int id;
};

std::ostream& operator<< (std::ostream& out, const MLSymbol r);

// ----------------------------------------------------------------
#pragma mark MLSymbolVector

// unused, a start.

// TODO replace all use of std::string with TextFragment. 

// store in memory pool, 1Mb or so in 4 byte increments or so. Never delete. When pool is full
// make another. Or just fail. 1Mb / 8 bytes : 128k strings.

// paths: will be lists of symbolvectors. Each vector can include spaces or other punctuation. 
// maybe make ID 1 = " " 

// arbitrary text can go into a symbolvector.

// in Chinese each character will be one symbol. 
// Japanese too (except phonetic characters?) 

class MLSymbolVector : public std::vector<MLSymbol> 
{
public:
	MLSymbolVector() {}
	MLSymbolVector(std::vector<MLSymbol> b) 
	{
		for(auto it = b.begin(); it != b.end(); ++it)
		{
			push_back(*it);
		}
	}
	~MLSymbolVector() {}
	
	inline bool operator< (const MLSymbolVector& b) const
	{
		const std::vector<MLSymbol>& a = *this;
		size_t aLen = size();
		size_t bLen = b.size();
		size_t minSize = std::min(aLen, bLen);
		for(int i=0; i<minSize; ++i)
		{
			MLSymbol symA = a[i];
			MLSymbol symB = b[i];
			if(symA < symB)
			{
				return true;
			}
			else if (symB < symA)
			{
				return false;
			}
		}
		// at this point the vectors are equal up to min length
		return aLen < bLen;
	}		
};

// ----------------------------------------------------------------
#pragma mark MLNameMaker
// a utility to make many short, unique, human-readable names when they are needed. 

class MLNameMaker
{
public:
	MLNameMaker() : index(0) {};
	~MLNameMaker() {};
	
	// return the next name as a symbol, having added it to the symbol table. 
	const MLSymbol nextName();
	
private:
	int index;
};

// MLTEST


class TestProc
{
public:
	
	TestProc(){}
	~TestProc(){}

	// template syntax here is needed to get the string literal length N at compile time.
	template<size_t N>
	inline void setParam(const char(&name)[N], float val)
	{
		std::cout << "setParam - HSL\n";
		HashedCharArray hsl(name);
		MLSymbol m(hsl);
		map[m] = val;
	}

	inline void setParam(MLSymbol name, float val)
	{
		std::cout << "setParam - MLSymbol\n";
		map[name] = val;
	}
	
	inline float getParam(const MLSymbol name)
	{
		return map[name];
	}
	
	std::map< MLSymbol, float > map;
	
};


} // namespace ml


// hashing function for MLSymbol use in unordered STL containers. simply return the ID,
// which gives each MLSymbol a unique hash.
namespace std
{
	template<>
	struct hash<ml::MLSymbol>
	{
		std::size_t operator()(ml::MLSymbol const& s) const
		{
			return s.id;
		}
	};
}
