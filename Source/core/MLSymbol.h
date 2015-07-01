
// MLSymbol.h
// ----------
// Madrona Labs C++ framework for DSP applications.
// Copyright (c) 2015 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/


#ifndef _ML_SYMBOL_H
#define _ML_SYMBOL_H

#include <set>
#include <list>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <stdlib.h>
#include <mutex>

//#include "MLDebug.h"

// MLTEST
#define debug() std::cout


// TODO document with doxygen!!



typedef int SymbolIDT;		// unique ID of symbol.
typedef int SymbolIndexT;	// sorted index of symbol.

static const int kMLMaxSymbolLength = 56;
static const int kMLMaxNumberLength = 8;


const int kHashTableBits = 16;
const int kHashTableSize = (1 << kHashTableBits);
const int kHashTableMask = kHashTableSize - 1;



class MLSymbolTable
{
public:
	MLSymbolTable();
	~MLSymbolTable();
	void clear();
	
	SymbolIndexT getSymbolAlphaOrder(const SymbolIDT symID);
	int addEntry(const char * sym);
	
	// look up a symbol by name and return its ID. Used in MLSymbol constructors.
	// if the symbol already exists, this routine must not allocate any memory.
	
	SymbolIDT getSymbolID(const char * sym);
	
	int getSize() { return mSymbolsByID.size(); }
	
	const std::string& getSymbolByID(SymbolIDT symID);
	
	void dump(void);
	void audit(void);
	
private:
	
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
	
	// TODO add option
	static const int kDefaultTableSize = 10000;
	
	int mSize;
	
	std::mutex mMutex;
	
	// each symbol is stored as a std::string.
	// a symbol's ID is defined as the order in which the symbol is created, and never changes. 
	// a symbol's index is the alphabetically sorted order of the symbol.
	
	// vector of symbols in ID order
	std::vector<std::string> mSymbolsByID;	
	
	// vector of alphabetically sorted indexes into symbol vector, in ID order
	std::vector<SymbolIndexT> mAlphaOrderByID;	
	
	// vector of indexes into symbol vector, in alphabetical order
	//std::vector<int> mIDsByIndex;	
	
	// TEMP set used for sorting.
	std::set<std::string> mSymbolsByIndex;
	
	// hash table containing indexes to strings
	std::vector< std::vector<int> > mHashTable;
	
};

inline MLSymbolTable& theSymbolTable()
{
	static std::unique_ptr<MLSymbolTable> t (new MLSymbolTable());
	return *t;
}

// ----------------------------------------------------------------
#pragma mark MLSymbol

// MLSymbol is designed to be an efficient key in map structures that is easy 
// to convert to and from a unique string.  The local value of an MLSymbol must 
// remain valid even after more MLSymbols are created.  
//
// This second requirement means that a layer of indirection is needed to get
// from MLSymbols to the sortable primitives associated with them.  That layer is the
// symbol table.  Each symbol has an ID, which remains constant for its lifetime. 
// Each ID can quickly reference a symbol index, which may change when new symbols
// are added. 

class MLSymbol
{
	friend std::ostream& operator<< (std::ostream& out, const MLSymbol r);
public:
	
	// create symbols:
	// Must be reasonably fast.  We will often want to be
	// lazy and write code like getParam("gain"), even in a DSP method. 
	// So the constructor must not allocate memory, even any
	// temporary memory, after the first time a symbol is used.  
	//
	// Where the best possible performance is needed, symbols can be
	// cached like so:
	//
	// void myDSPMethod() {
	//		static const MLSymbol gainSym("gain");
	//		...
	//		getParam(gainSym);
	//
	MLSymbol();
	MLSymbol(const char *sym);
	MLSymbol(const std::string& str);
	
	// compare two symbols:
	// Must be the fastest. used in std:map all over the place.
	// bool operator< (const MLSymbol b) const;

	
	inline bool operator< (const MLSymbol sb) const
	{
		
		int a = mID;
		int b = sb.mID;
		int c = theSymbolTable().getSymbolAlphaOrder(a);
		int d = theSymbolTable().getSymbolAlphaOrder(b);
		return (c < d);
		
		
		// no need for alpha sort?!		
		//return (mID < sb.mID);
	}

	
	// bool operator== (const MLSymbol b) const;	
	inline bool operator== (const MLSymbol b) const
	{
		return (mID == b.mID);
	}
	
	
	operator bool() const { return mID != 0; }
	
	bool beginsWith (const MLSymbol b) const;
	bool endsWith (const MLSymbol b) const;
	
	bool hasWildCard() const;
	MLSymbol withWildCardNumber(int n) const;
	MLSymbol withFinalNumber(int n) const;
	MLSymbol withoutFinalNumber() const;
	int getFinalNumber() const;
	
	// TODO an idea: getTranslatedString() here might be a good API for internationalization
	// a dictionary of symbols -> translations (incl. English) is needed. 
	
	const std::string& getString() const;
	int compare(const char *str) const;
	
	std::size_t getID() const { return mID; }
	
private:
	
	// this private ID will reflect the order in which the symbol was created.
	SymbolIDT mID;
};

std::ostream& operator<< (std::ostream& out, const MLSymbol r);

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

// ----------------------------------------------------------------
#pragma mark MLNameMaker
// a utility to make many short, human-readable names when they are needed. 

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



#endif // _ML_SYMBOL_H

