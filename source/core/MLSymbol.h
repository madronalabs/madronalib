
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

#ifndef _ML_SYMBOL_H
#define _ML_SYMBOL_H

#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <algorithm>

#include "MLLocks.h"

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

constexpr uint32_t KRhashConst(const char *s)
{
	/*
	 const unsigned char *p;
	 unsigned hashval = 0;
	 for (p = (const unsigned char *) s; *p; p++)
	 {
		hashval = *p + 31u * hashval;
	 }*/
	return 31;
}

class HashedStringLiteral
{
public:	
	template<int N>
	constexpr HashedStringLiteral(const char (&sym)[N]) : hash(KRhashConst(sym)), pSym(sym), len(N) {}
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
	int getSize() { return mSize; }	
	void dump(void);
	int audit(void);
	
protected:
	// look up a symbol by name and return its ID. Used in MLSymbol constructors.
	// if the symbol already exists, this routine must not allocate any heap memory.
	int getSymbolID(const char * sym);
	
	int getSymbolID(HashedStringLiteral hsl);
	
	const std::string& getSymbolByID(int symID);
	int addEntry(const char * sym, int len);
#if USE_ALPHA_SORT	
	int getSymbolAlphaOrder(const int symID);
#endif
	
private:
	void allocateChunk();

	// very simple hash function from Kernighan & Ritchie.
	inline unsigned KRhash(const char *s)
	{
		const unsigned char *p;
		unsigned hashval = 0;
		for (p = (const unsigned char *) s; *p; p++)
		{
			hashval = *p + 31u * hashval;
		}
		return hashval & kHashTableMask;
	}
	
	// ensure symbol table integrity with simple SpinLock.
	MLSpinLock mLock;
	
	// 2^31 unique symbols are possible. There is no checking for overflow.
	int mSize;
	int mCapacity;
	
	// vector of symbols in ID/creation order
	std::vector< std::string > mSymbolsByID;	
	
	// hash table containing indexes to strings
	std::vector< std::vector<int> > mHashTable;
	
#if USE_ALPHA_SORT	
	// vector of alphabetically sorted indexes into symbol vector, in ID order
	std::vector<int> mAlphaOrderByID;	
	
	// std::set is used for sorting.
	std::set<std::string, MLStringCompareFn> mSymbolsByAlphaOrder;
#endif

};

inline MLSymbolTable& theSymbolTable()
{
	static std::unique_ptr<MLSymbolTable> t (new MLSymbolTable());
	return *t;
}

// ----------------------------------------------------------------
#pragma mark MLSymbol

// this template juju comes courtesy of ansiwen on stackoverflow.
// its purpose is to lower the precedence of the const char* constructor
// in order for us to make a special constructor for string literals,
// which have type const char (&)[N].

#define BARK std::cout << __PRETTY_FUNCTION__ << std::endl

struct Dummy {};
template<typename T> struct IsCharPtr {};
template<> struct IsCharPtr<const char *> { typedef Dummy* Type; };
template<> struct IsCharPtr<char *> { typedef Dummy* Type; };

struct Foo 
{
	template<int N> Foo(const char (&)[N]) { BARK; }
	template<int N> Foo(char (&)[N]) { BARK; }
	template<typename T> Foo(T, typename IsCharPtr<T>::Type=0) { BARK; }
};

class MLSymbol
{
	friend std::ostream& operator<< (std::ostream& out, const MLSymbol r);
	
public:
	
	// creating symbols:
	// Must be reasonably fast.  We will often want to be
	// lazy and write code like getParam("gain"), even in a DSP method. 
	// So the constructor must not allocate any heap memory when looking up
	// a symbol, after the first time a symbol is used.  
	//
	// Where the best possible performance is needed, symbols can be
	// cached like so:
	//
	// void myDSPMethod() {
	//		static const MLSymbol gainSym("gain");
	//		...
	//		getParam(gainSym);
	
	// can definitely do a constexpr hash at compile time. 
	// but what about forcing table addition at static init time for const char * ? 

	MLSymbol() : mID(0) {}
	
	/*
	MLSymbol(const char* sym) : mID(theSymbolTable().getSymbolID(sym)) { std::cout << " L "; }
	MLSymbol(char *sym) : mID(theSymbolTable().getSymbolID(sym)) { std::cout << " N "; }
	*/
	
	class SymbolID
	{
	public:
		explicit SymbolID(HashedStringLiteral hsl) : val(theSymbolTable().getSymbolID(hsl)) {}
		explicit SymbolID(const char* sym) : val(theSymbolTable().getSymbolID(sym)) {}
		int val;
	};
	
	// template for creating MLSymbol from a string literal.
	// creates the hash at compile time.
	template<int N> MLSymbol(const char (&sym)[N]) : 
	mID(HashedStringLiteral(sym))
	{ std::cout << " LIT "; } 
	
//	template<int N> constexpr MLSymbol(const char (&sym)[N]) : mID(ctcrc32(sym)) { } 
//	MLSymbol(HashedStringLiteral hsl) : mID(hsl.hash) { } 
	
	// initfromhash
	// TODO finish creation after hash
	
	template<int N> MLSymbol(char (&sym)[N]) : 
	mID(sym)
	{  std::cout << " NC "; }
	
	template<typename T> MLSymbol(T sym, typename IsCharPtr<T>::Type=0) : 
	mID(sym)
	{ std::cout << " O "; BARK; }
	
	
//	MLSymbol(const std::string& str);
	
	inline bool operator< (const MLSymbol b) const
	{
#if USE_ALPHA_SORT			
		return (theSymbolTable().getSymbolAlphaOrder(mID) < theSymbolTable().getSymbolAlphaOrder(b.mID));
#else
		return(mID.val < b.mID.val);
#endif
	}
	
	inline bool operator== (const MLSymbol b) const
	{
		return (mID.val == b.mID.val);
	}	
	
	inline bool operator!= (const MLSymbol b) const
	{
		return (mID.val != b.mID.val);
	}	
	
	explicit operator bool() const { return mID.val != 0; }
	inline int getID() const { return mID.val; }
	
	// in order to show the strings in XCode's debugger, instead of the unhelpful mID,
	// edit the summary format for MLSymbol within XCode to {$VAR.getString()}:s
	const std::string& getString() const;
	
	bool beginsWith (const MLSymbol b) const;
	bool endsWith (const MLSymbol b) const;
	bool hasWildCard() const;
	
	int getFinalNumber() const;	
	int compare(const char *str) const;
	
	// TODO make free functions
	MLSymbol append(const std::string& b) const;
	MLSymbol withWildCardNumber(int n) const;
	MLSymbol withFinalNumber(int n) const;
	MLSymbol withoutFinalNumber() const;
	
private:
	
	// the ID equals the order in which the symbol was created.
	const SymbolID mID;
};

std::ostream& operator<< (std::ostream& out, const MLSymbol r);

// ----------------------------------------------------------------
#pragma mark MLSymbolVector

// unused, a start.

// TODO replace all use of std::string with symbol vectors. 

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
	
	// return the next name in the sequence as a string. 
	std::string nextNameAsString();
	
	// return the next name as a symbol, having added it to the symbol table. 
	const MLSymbol nextName();
	
private:
	int index;
};

// hashing function for MLSymbol use in unordered STL containers. simply return the ID,
// which will give each MLSymbol a unique hash.

namespace std
{
	template<>
	struct hash<MLSymbol>
	{
		std::size_t operator()(MLSymbol const& s) const
		{
			return s.getID();
		}
	};
}


// MLTEST

#include <map>

class TestProc
{
public:
	
	TestProc(){}
	~TestProc(){}
	
	inline void setParam(const MLSymbol name, float val)
	{
		map[name] = val;
	}
	
	inline float getParam(const MLSymbol name)
	{
		return map[name];
	}
	
	std::map< MLSymbol, float > map;
	
};




#endif // _ML_SYMBOL_H

