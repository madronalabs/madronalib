
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_SYMBOL_H
#define _ML_SYMBOL_H

#include <list>
#include <string>
#include <map>
#include <iostream>
#include <stdlib.h>
#include "MLDebug.h"

typedef unsigned short SymbolIDT;		// unique ID of symbol.
typedef unsigned short SymbolIndexT;	// sorted index of symbol.

static const int kMLMaxSymbolLength = 56;
static const int kMLMaxNumberLength = 8;

// ----------------------------------------------------------------
#pragma mark MLCharArray

// an MLSymbolKey points to a number of characters at a certain location.
// the number is needed because we will compare keys with portions of
// static strings: see MLSymbol::MLSymbol.

// used as key in symbol table structures so that allocating strings is not necessary 
// for looking up symbol IDs. 
class MLSymbolKey
{
public:
	MLSymbolKey(const char * data, int len);
	~MLSymbolKey();
	
	// make a local copy of the external char array we are referencing.
	void makeString();
	
	bool operator< (const MLSymbolKey& b) const;
	
	const char * mpData;
	int mLength;
	std::string* mpString;
};

// ----------------------------------------------------------------
#pragma mark MLSymbolTable

class MLSymbolTable
{
public:
	MLSymbolTable();
	~MLSymbolTable();
	
	// must be fastest.
	inline SymbolIndexT getSymbolIndex(const SymbolIDT symID) const
	{
		return mIndexesByID[symID];
	}
	
	SymbolIDT getSymbolID(const char * sym, const int len);
	const std::string& getStringByID(SymbolIDT symID);
	SymbolIDT getID(SymbolIDT symID);
	void dump(void);
	void audit(void);
	
private:
	static const unsigned int kTableSize = 1 << (sizeof(SymbolIDT)*8);
	
	std::string mNullString;
	
	// indexes into strings list, 128kB of storage in this impl
	SymbolIndexT mIndexesByID[kTableSize];	

	// ptrs to strings in keys, 256kB of storage in this impl
	std::string* mStringsByID[kTableSize];	

	typedef std::map<MLSymbolKey, SymbolIDT> MLSymbolMapT;
	MLSymbolMapT mMap;
};


inline MLSymbolTable& theSymbolTable()
{
	static MLSymbolTable* t = new MLSymbolTable();
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
//
// Passing MLSymbols by value should be the norm.  Currently sizeof(MLSymbol)
// is two bytes, which allows for 2^16 symbols in the table.  

class MLSymbol
{
friend std::ostream& operator<< (std::ostream& out, const MLSymbol r);
public:

	// create symbols:
	// Must be reasonably fast.  We will often want to be
	// lazy and write code like getParam("gain"), even in a DSP method. 
	// So the constructor must not allocate memory, even any
	// temporary memory, after the first time a signal is used.  
	//
	// Where the best possible performace is needed, symbols can be
	// cached like so:
	//
	// void myDSPMethod() {
	//		static const MLSymbol gainSym("gain");
	//		...
	//		getParam(gainSym);
	//
	MLSymbol();
	MLSymbol(const char *sym);
	MLSymbol(const char *sym, int maxLen);
	MLSymbol(const std::string& sym);
	MLSymbol(SymbolIDT id) : mID(id) {}

	// compare two symbols:
	// Must be the fastest. used in std:map all over the place.
	bool operator< (const MLSymbol b) const;
	
	bool operator== (const MLSymbol b) const;
	operator bool() const { return mID != 0; }
	
	bool hasWildCard() const;
	MLSymbol withWildCardNumber(int n) const;
	MLSymbol withFinalNumber(int n) const;
	MLSymbol withoutFinalNumber() const;
	int getFinalNumber() const;

	const std::string& getString() const;
	int compare(const char *str) const;
	
private:
	SymbolIDT mID;
};

inline bool MLSymbol::operator< (const MLSymbol b) const
{
	return ((theSymbolTable().getSymbolIndex(mID)) < (theSymbolTable().getSymbolIndex(b.mID)));
}

inline bool MLSymbol::operator== (const MLSymbol b) const
{
	return (mID == b.mID);
}

std::ostream& operator<< (std::ostream& out, const MLSymbol r);


// ----------------------------------------------------------------
#pragma mark MLNameMaker
// a utility to make many short names when they are needed. 

class MLNameMaker
{
public:
	MLNameMaker() : index(0) {};
	~MLNameMaker() {};
	const MLSymbol nextName();

private:
	int index;
	
};



#endif // _ML_SYMBOL_H

