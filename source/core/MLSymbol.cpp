
// MLSymbol.cpp
// ----------
// Madrona Labs C++ framework for DSP applications.
// Copyright (c) 2015 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSymbol.h"

#pragma mark utilities

namespace ml {

const int kMLMaxNumberDigits = 14;

const char *positiveIntToDigits(int i);
int digitsToPositiveInt(const char* p);
const char *naturalNumberToDigits(int value, char* pDest);
bool isDigit(char c);

const char *positiveIntToDigits(int i)
{
	static char buf[kMLMaxNumberDigits + 2] = {0};
	char *p = buf + kMLMaxNumberDigits + 1;	
	*p = 0;
	do 
	{
		*--p = '0' + (i % 10);
		i /= 10;
	} 
	while (i != 0);
	return p;
}

const char* naturalNumberToDigits(int value, char* pDest) 
{
	const int base = 10;
	char* ptr = pDest, *ptr1 = pDest, tmp_char;
	int tmp_value;
	
	if(value <= 0)
	{
		*pDest = '0';
		*(++pDest) = '\0';
		return pDest;
	}
	do 
	{
		tmp_value = value;
		value /= base;
		*ptr++ = "0123456789abcdef"[tmp_value - value*base];
	} while ( value > 0 );
	
	*ptr-- = '\0';
	while(ptr1 < ptr) 
	{
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return pDest;
}

int digitsToPositiveInt(const char* p)
{
	int v = 0;
	int l = 0;
	int d;
	char c;
	
	while (p[l] && (l < kMLMaxNumberDigits-1))
	{
		c = p[l];
		if (c >= '0' && c <= '9')
			d = (c - '0');
		else
			break;
		v = (v * 10) + d;
		l++;
	}
	return v;
}

bool isDigit(char c)
{
	if (c >= '0' && c <= '9')
		return true;
	return false;
}

std::ostream& operator<< (std::ostream& out, const Symbol r)
{
	out << r.getTextFragment();
	return out;
}


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
	MLScopedLock lock(mLock);
	mSymbolsByID.clear();
	
	mHashTable.clear();
	mHashTable.resize(kHashTableSize);	
	addEntry("", 0);
}

// add an entry to the table. The entry must not already exist in the table.
// this must be the only way of modifying the symbol table.
int SymbolTable::addEntry(const char * sym, uint32_t hash)
{
	mSymbolsByID.emplace_back(TextFragment(sym));
	
	size_t newID = mSymbolsByID.size() - 1;
	mHashTable[hash].push_back(newID);	
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
			if(compareTextFragmentToChars(mSymbolsByID[ID], hsl.pSym))
			{
				r = ID;
				found = true;
				break;
			}
		}
		
		if(!found)
		{	
			r = addEntry(hsl.pSym, hsl.hash);
		}
	}
	
	return r;
}

int SymbolTable::getSymbolID(const char * sym)
{
	return getSymbolID(HashedCharArray(sym));
}

const TextFragment& SymbolTable::getSymbolByID(int symID)
{
	return mSymbolsByID[symID];
}

void SymbolTable::dump()
{
	std::cout << "---------------------------------------------------------\n";
	std::cout << mSymbolsByID.size() << " symbols:\n";
		
	// print symbols in order of creation. 
	for(int i=0; i<mSymbolsByID.size(); ++i)
	{
		const TextFragment& sym = mSymbolsByID[i];
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
				std::cout << id << " ";
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
	size_t size = mSymbolsByID.size();
 
	for(i=0; i<size; ++i)
	{
		const TextFragment& sym = getSymbolByID(i);
		const char* symChars = sym.text;
		Symbol symB(symChars);

		i2 = symB.id;
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
		const TextFragment& s = getSymbolByID(i);
		std::cout << "SymbolTable: error in symbol table, line " << i << ":\n";
		std::cout << "    ID " << i << " = " << s << ", ID B = " << i2 << "\n";
	}
	return OK;
}


#pragma mark Symbol

// return a reference to the symbol's TextFragment in the table.
const TextFragment& Symbol::getTextFragment() const
{
	return theSymbolTable().getSymbolByID(id);
}

/*
bool Symbol::beginsWith (const Symbol b) const
{
	const std::string& strA = getString();
	const char* pa = strA.c_str();
	const size_t aLen = strA.length();
	const std::string& strB = b.getString();
	const char* pb = strB.c_str();
	const size_t bLen = strB.length();
	
	if(bLen > aLen) return false;
	for(int i=0; i<bLen; ++i)
	{
		if(pa[i] != pb[i]) return false;
	}
	return true;
}

bool Symbol::endsWith (const Symbol b) const
{
	const std::string& strA = getString();
	const char* pa = strA.c_str();
	const size_t aLen = strA.length();
	const std::string& strB = b.getString();
	const char* pb = strB.c_str();
	const size_t bLen = strB.length();
	
	if(bLen > aLen) return false;
	for(size_t i = bLen; i > 0; --i)
	{
		if(pa[i + aLen - bLen - 1] != pb[i - 1]) return false;
	}
	return true;
}
*/

/*
Symbol Symbol::append(const std::string& b) const
{
	return Symbol(getString() + std::string(b));
}
*/

bool Symbol::hasWildCard() const
{
	const TextFragment& str = getTextFragment();
	const char* p = str.text;
	int l = 0;
	while(p[l] && (l < kMLMaxSymbolLength-1))
	{
		if (p[l++] == '*')
			return true;
	}
	return false;
}

/*
// replace wild card with the given number.
Symbol Symbol::withWildCardNumber(int n) const
{
	const std::string& inStr = getString();
	const char* p = inStr.c_str();
	int l = 0;
	int m = 0;
	char c;
	char tempBuf[kMLMaxSymbolLength] = {0};
	
	while(p[l] && (m < kMLMaxSymbolLength-1))
	{
		c = p[l];
		if (c == '*')
		{
			const char* d = positiveIntToDigits(n);
			int j = 0;
			while(d[j] && (m < kMLMaxSymbolLength-1))
			{
				tempBuf[m] = d[j];
				j++;
				m++;
			}
			m--;
		}
		else
		{
			tempBuf[m] = c;
		}
		l++;
		m++;
	}
	tempBuf[m] = 0;
	return Symbol(tempBuf);
}
*/

/*
// if the symbol's string ends in a number, return that number.
int Symbol::getFinalNumber() const
{
	const std::string& str = getString();
	const char* p = str.c_str();
	int l = 0;
	while(p[l])
	{
		l++;
	}
	if (!isDigit(p[l - 1])) return 0; // no ending number
	char c;
	int i;
	for (i = l-1; i >= 0; i--)
	{
		c = p[i];
		if (!isDigit(c)) break;
	}
	return digitsToPositiveInt(&p[i + 1]);
}

// add a number n to the end of a symbol after removing any final number.
// n must be >= 0.
Symbol Symbol::withFinalNumber(int n) const
{
	// *not* static because different threads could collide accesing a single buffer.
	char tempBuf[kMLMaxSymbolLength + kMLMaxNumberLength] = {0};
	
	const std::string& str = getString();
	const char* p = str.c_str();
	
	int i, j;
	int l = 0;
	
	// set l to length
	while(p[l])
	{
		l++;
	}
	// find last non-number character
	for (i = l-1; i >= 0; --i)
	{
		if (!isDigit(p[i])) break;
	}	
	// copy symbol without number to temp 
	for(j=0; j<=i; ++j)
	{
		tempBuf[j] = p[j];
	}
	
	naturalNumberToDigits(n, tempBuf+j);
	
	//debug() << "in: " << n << ", char*: " << tempBuf << ", Symbol " << Symbol(tempBuf) << "\n";	
	return Symbol(tempBuf);
}

// remove any final number.
Symbol Symbol::withoutFinalNumber() const
{
	// *not* static because different threads could collide accessing a single buffer.
	char tempBuf[kMLMaxSymbolLength] = {0};
	
	const std::string& str = getString();
	const char* p = str.c_str();
	int i, j;
	int l = 0;
	
	// set l to length
	while(p[l])
	{
		l++;
	}
	// find last non-number character
	for (i = l-1; i >= 0; --i)
	{
		if (!isDigit(p[i])) break;
	}	
	// copy symbol without number to temp 
	for(j=0; j<=i; ++j)
	{
		tempBuf[j] = p[j];
	}
	tempBuf[i+1] = 0;
	return Symbol(tempBuf);
}
*/

/*
int Symbol::compare(const char * s) const
{
	return getString().compare(s);
}*/

#pragma mark MLNameMaker


// base-26 arithmetic with letters (A = 0) produces A, B, ... Z, BA, BB ...
const Symbol MLNameMaker::nextName()
{
//	std::string nameStr;
	std::vector<int> digits;
	
	const int base = 26;
	const int maxLen = 64;
	static char buf[maxLen];
	const char baseChar = 'A';
	int a, m, d, rem;
	
	a = index++;
	
	if (!a)
	{
		digits.push_back(0);
	}
	else while(a)
	{
		d = a / base;
		m = d * base;
		rem = a - m;
		digits.push_back(rem);
		a = d;
	}
	
	int c = 0;
	while(digits.size() && (c < maxLen))
	{
		d = digits.back();
		digits.pop_back();
		
		buf[c++] = static_cast<char>(d) + baseChar;
		// nameStr += (char)d + baseChar;
	}
	// TODO 
	return Symbol(buf);
}
	
} // namespace ml
