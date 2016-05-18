
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



#define BARK std::cout << __PRETTY_FUNCTION__ << std::endl

struct Dummy {};
template<typename T> struct IsCharPtr {};
template<> struct IsCharPtr<const char *> { typedef Dummy* Type; };
template<> struct IsCharPtr<char *> { typedef Dummy* Type; };

struct Foo {
	template<int N> Foo(const char (&)[N]) { BARK; }
	template<int N> Foo(char (&)[N]) { BARK; }
	template<typename T> Foo(T, typename IsCharPtr<T>::Type=0) { BARK; }
};


namespace detail {
	// CRC32 Table (zlib polynomial)
	static constexpr uint32_t crc_table[256] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
		0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
		0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
		0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
		0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
		0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
		0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
		0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
		0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
		0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
		0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};
	
	template<size_t idx>
	constexpr uint32_t combine_crc32(const char * str, uint32_t part) {
  return (part >> 8) ^ crc_table[(part ^ str[idx]) & 0x000000FF];
	}
	
	template<size_t idx>
	constexpr uint32_t crc32(const char * str) {
  return combine_crc32<idx>(str, crc32<idx - 1>(str));
	}
	
	// This is the stop-recursion function
	template<>
	constexpr uint32_t crc32<size_t(-1)>(const char * str) {
  return 0xFFFFFFFF;
	}
	
} //namespace detail
	
template <size_t len>
constexpr uint32_t ctcrc32(const char (&str)[len]) 
{
	return detail::crc32<len - 2>(str) ^ 0xFFFFFFFF;
}


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

	static constexpr int testConst(void) 
	{ 
		return 2; 
	}
	
	constexpr MLSymbol() : mID(0) {}
	
	/*
	MLSymbol(const char* sym) : mID(theSymbolTable().getSymbolID(sym)) { std::cout << " L "; }
	MLSymbol(char *sym) : mID(theSymbolTable().getSymbolID(sym)) { std::cout << " N "; }
	*/
	
	class HashedStringLiteral
	{
	public:	
		constexpr HashedStringLiteral(const char *sym) : hash(1), pSym(sym), len(2) {}

		int32_t hash;
		const char* pSym;
		size_t len;
	};

	class SymbolID
	{
	public:
		explicit constexpr SymbolID(HashedStringLiteral hsl) : val(3) {}
		explicit constexpr SymbolID(int id) : val(id) {}
		int val;
	};
	
	template<int N> MLSymbol(const char (&sym)[N]) : mID(HashedStringLiteral(sym)) { std::cout << " LIT "; } 
	
//	template<int N> constexpr MLSymbol(const char (&sym)[N]) : mID(ctcrc32(sym)) { } 
//	MLSymbol(HashedStringLiteral hsl) : mID(hsl.hash) { } 
	
	// initfromhash
	// TODO finish creation after hash
	
	template<int N> MLSymbol(char (&sym)[N]) : mID(theSymbolTable().getSymbolID(sym)) {  std::cout << " NC "; }
	template<typename T> MLSymbol(T sym, typename IsCharPtr<T>::Type=0) : mID(theSymbolTable().getSymbolID(sym)) { std::cout << " O "; BARK; }
	

	
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

