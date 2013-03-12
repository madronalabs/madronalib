
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLSymbol.h"
#include <iostream>
#include <sstream>

// ----------------------------------------------------------------
#pragma mark MLSymbolKey

MLSymbolKey::MLSymbolKey(const char * data, unsigned len) : 
	mpData(data), mLength(len), mpString(0) 
{
}

MLSymbolKey::~MLSymbolKey() 
{
	if (mpString) delete(mpString);
}

void MLSymbolKey::makeString()
{
	if (mLength && mpData)
	{
		if (mpString) delete(mpString);
		mpString = new std::string(mpData, mLength);
		if (!mpString)
		{
			MLError() << "MLSymbolKey::makeString: could not allocate!\n";
		}
		else
		{
			mpData = mpString->data();
//			debug() << "made string " << *mpString << "\n";
		}
	}
}

bool MLSymbolKey::operator< (const MLSymbolKey & b) const
{
	bool r = false;
	unsigned la = mLength;
	unsigned lb = b.mLength;
	
	// len is smaller of the two key lengths.
	unsigned len = (la < lb) ? la : lb;
	
	// compare up to len
	bool equal = true;
	for(unsigned c=0; c<len; ++c)
	{
		char ca = mpData[c];
		char cb = b.mpData[c];
		if (ca != cb)
		{
			r = (ca < cb);
			equal = false;
			break;
		}
	}	
	
	// if strings are equal up to len, whichever key has fewer chars is less.
	if(equal)
	{
		r = (la < lb);
	}

	return r;
}



// ----------------------------------------------------------------
#pragma mark MLSymbolTable

MLSymbolTable::MLSymbolTable()
{
	mNullString = "[]";
	
	MLSymbolMapT::iterator newEntryIter;
	std::pair<MLSymbolMapT::iterator, bool> newEntryRet;
	
	// make null symbol 
	// insert key/ID pair into map, with ID=0 
	MLSymbolKey symKey("", 0);	
	std::pair<MLSymbolKey, SymbolIDT> newEntry(symKey, 0);
	newEntryRet = mMap.insert(newEntry);
	newEntryIter = newEntryRet.first;

	// make key data local in map
	MLSymbolKey& newKey = const_cast<MLSymbolKey&>(newEntryIter->first);			
	
	// make null string
	newKey.mpString = new std::string();
	newKey.mpData = newKey.mpString->data();
	
	// make new index list entry
	mIndexesByID[0] = 0;
	mStringsByID[0] = newKey.mpString;			
}


MLSymbolTable::~MLSymbolTable()
{

}

// look up a symbol by name and return ID.
// if the symbol already exists, this routine must not allocate any memory.
SymbolIDT MLSymbolTable::getSymbolID(const char * sym, const int len)
{
	SymbolIDT r = 0;
	bool found = false;
	unsigned size = mMap.size();
	
//debug() << size << " entries, making symbol " << sym << "\n";
	
	if (len == 0)
	{
		return 0;
	}	

	// symbol data stays external for now, no memory is allocated.
	MLSymbolKey symKey(sym, len);	
	MLSymbolMapT::iterator mapIter(mMap.find(symKey));
	if (mapIter != mMap.end())
	{
		found = true;
		r = mapIter->second;
	}

	if (!found)
	{
		// make a new entry in the symbol table.
		if(size < kTableSize)
		{		
			MLSymbolMapT::iterator beginIter;
			MLSymbolMapT::iterator newEntryIter;
			std::pair<MLSymbolMapT::iterator, bool> newEntryRet;
			
			// insert key/ID pair into map, with ID=0 for now
			std::pair<MLSymbolKey, SymbolIDT> newEntry(symKey, 0);
			newEntryRet = mMap.insert(newEntry);
			
			// check insertion
			if (!newEntryRet.second)
			{
				MLError() << "MLSymbolTable::getSymbolID: error, key " << sym << " already in map!\n";
			}
			newEntryIter = newEntryRet.first;
			beginIter = mMap.begin();

			// get index of new entry
			unsigned newIndex = distance(beginIter, newEntryIter);
			
//debug() << "adding symbol " << sym << ", length " << len << "\n";		
//debug() << "new map entry index: " << newIndex << " ID = " << size << "\n";
		
			// make key data local in map
			MLSymbolKey& newKey = const_cast<MLSymbolKey&>(newEntryIter->first);			
			newKey.makeString();
			
			// set ID of new entry
			newEntryIter->second = size;
		
			// adjust indexes to reflect insertion
			for(unsigned id=0; id<size; ++id)
			{
				if (mIndexesByID[id] >= newIndex)
				{
					mIndexesByID[id]++;
				}
			}
			
			// make new index list entry
			mIndexesByID[size] = newIndex;	
			
			// make new string pointer. 
			mStringsByID[size] = newKey.mpString;
			
			r = size;	
		}
		else
		{
			debug() << "symbol table size exceeded!\n";
		}
	}
	
	return r;
}


const std::string& MLSymbolTable::getStringByID(SymbolIDT symID)
{
	return *mStringsByID[symID];
}

// for testing.  
SymbolIDT MLSymbolTable::getID(SymbolIDT symID)
{
	const SymbolIndexT symIdx = mIndexesByID[symID];
	
	MLSymbolMapT::iterator mapIter = mMap.begin();	
	advance(mapIter, symIdx);
	return (mapIter->second);	
}
	
void MLSymbolTable::dump(void)
{
	bool found;
	unsigned i, idx;
	unsigned size = mMap.size();
debug() << "---------------------------------------------------------\n";
debug() << size << " symbols:\n";

	// print in sorted order.
	for(idx=0; idx<size; ++idx)
	{
		found = false;
		for(i=0; i<size; ++i)
		{
			unsigned idxAtI = mIndexesByID[i];
			if (idx == idxAtI)
			{
				found = true;
				break;
			}
		}
		
		if (found)
		{
			const std::string& s = getStringByID(i);
			SymbolIDT id = getID(i);
debug() << "    ID " << i << ": index "  << idx << " = " << s << ", ID = " << id << "\n";
		}
		else
		{
			MLError() << "error: symbol index " << idx << "not found! \n";
		}
	}
}
	
		
void MLSymbolTable::audit(void)
{
	unsigned i=0;
	bool OK = true;
	unsigned size = mMap.size();

	for(i=0; i<size; ++i)
	{
		SymbolIDT id = getID(i);
		if (i != id)
		{
			OK = false;
			break;
		}
	}
	if (OK)
	{
//		debug() << ": symbol table OK. " << size << " symbols.\n";
	}
	else
	{
		unsigned idx = mIndexesByID[i];		
		SymbolIDT id = getID(i);
		const std::string& s = getStringByID(i);
		MLError() << "MLSymbolTable: error in symbol table, line " << i << ":\n";
		MLError() << "    ID " << i << ": index "  << idx << " = " << s << ", ID = " << id << "\n";
	}
}


// ----------------------------------------------------------------
#pragma mark MLSymbol

const int kMLMaxNumberDigits = 20; 

const char *positiveIntToDigits(int i);
int digitsToPositiveInt(const char* p);

const char *naturalNumberToDigits(int value, char* pDest);

bool isDigit(char c);
bool isValidSymbolChar(char c);
int processSymbolText(const char* sym, int maxLen = kMLMaxSymbolLength);

const char *positiveIntToDigits(int i)
{
	static char buf[kMLMaxNumberDigits + 2];
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
	register const int base = 10;
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

bool isValidSymbolChar(char c)
{
	if (c >= 'a' && c <= 'z')
		return true;
	if (c >= 'A' && c <= 'Z')
		return true;
	if (isDigit(c))
		return true;
	if (c == '_' || c == '*' || c == '#')
		return true;
	return false;
}

// process incoming symbol text in place up to maxLen characters and return the text length.  
//
int processSymbolText(const char* sym, int maxLen)
{
	int len = 0;
	// check for starting number (these will be invalid symbols)
	if (isDigit(sym[0]))
	{
		debug() << "processSymbolText warning: looking for symbol, found number " << sym << "  \n"; 
	}
	else
	{
		int n;
		for(n=0; (isValidSymbolChar(sym[n]) && (n < maxLen)); n++){}
		len = n;
		if (len >= kMLMaxSymbolLength)
		{
			debug() << "processSymbolText warning: symbol exceeded max size! \n"; 
		}
	}
	return len;
}

MLSymbol::MLSymbol()
{
	mID = 0;
}

MLSymbol::MLSymbol(const char *sym)
{
	int len = processSymbolText(sym);
	mID = theSymbolTable().getSymbolID(sym, len);
}

MLSymbol::MLSymbol(const std::string& str)
{	
	int len = processSymbolText(str.c_str());
	mID = theSymbolTable().getSymbolID(str.c_str(), len);
}

MLSymbol::MLSymbol(const char *sym, int maxLen)
{
	int len = processSymbolText(sym, maxLen);
	mID = theSymbolTable().getSymbolID(sym, len);
}

// return a reference to the symbol's string in the table.
const std::string& MLSymbol::getString() const
{
	return theSymbolTable().getStringByID(mID);
}

bool MLSymbol::hasWildCard() const
{
	const std::string& str = getString();
	const char* p = str.c_str();
	int l = 0;
	while(p[l] && (l < kMLMaxSymbolLength-1))
	{
		if (p[l++] == '*')
			return true;
	}
	return false;
}

// replace wild card with the given number.
MLSymbol MLSymbol::withWildCardNumber(int n) const
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
	return MLSymbol(tempBuf);
}

// if the symbol's string ends in a number, return that number.
int MLSymbol::getFinalNumber() const
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
MLSymbol MLSymbol::withFinalNumber(int n) const
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
	
	//debug() << "in: " << n << ", char*: " << tempBuf << ", Symbol " << MLSymbol(tempBuf) << "\n";	
	return MLSymbol(tempBuf);
}


// remove any final number.
MLSymbol MLSymbol::withoutFinalNumber() const
{
	// *not* static because different threads could collide accesing a single buffer.
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
	return MLSymbol(tempBuf);
}


int MLSymbol::compare(const char * s) const
{
	return getString().compare(s);
}

std::ostream& operator<< (std::ostream& out, const MLSymbol r)
{
	out << r.getString();
	return out;
}


// ----------------------------------------------------------------
#pragma mark MLNameMaker

// base-26 arithmetic with letters produces A, B, ... Z, AA, AB ...
const MLSymbol MLNameMaker::nextName()
{
	std::string nameStr;
	unsigned base = 26;
	char baseChar = 'A';
	unsigned a, m, d, rem;
	
	std::list<unsigned> digits;
	a = index++;
	
	if (!a)
	{
		digits.push_front(0);
	}
	else while(a)
	{
		d = a / base;
		m = d * base;
		rem = a - m;
		digits.push_front(rem);
		a = d;
	}
		
	while(digits.size())
	{
		d = digits.front();
		digits.pop_front();
		nameStr += (char)d + baseChar;
	}
		
	return MLSymbol(nameStr.c_str());
}




