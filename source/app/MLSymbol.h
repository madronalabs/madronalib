
// ml::Symbol.h
// ----------

// ml::Symbol is designed to be an efficient key in STL containers such as map and
// unordered_map, that is quick to convert to and from a unique UTF-8 string.
//
// requirements
// ---------
//
// Symbols are immutable.
// The value of a Symbol must remain valid even after more ml::Symbols are created.
// This allows ml::Symbols to function as keys in any kind of data structure.
//
// Accessing a Symbol must not cause any heap to be allocated if the symbol already exists.
// This allows use in DSP code, assuming that the signal graph or whatever has already been parsed.
//
// Symbols must not ever require any heap as long as they are smaller than a certain size.
// Currently this relies on the "small string optimization" implementation of TextFragment.
// Currently the size is 16 bytes.

#pragma once

#include <array>
#include <vector>
#include <iostream>
#include <cstring>
#include <mutex>

#include "MLText.h"

namespace ml
  {
  constexpr int kHashTableBits = 12;
  constexpr int kHashTableSize = (1 << kHashTableBits);
  constexpr int kHashTableMask = kHashTableSize - 1;
  
  // initial capacity of symbol table. if this number of symbols is exceeded, the capacity of
  // the table will have to be increased, which may result in a glitch if called from the audio thread.
  // TODO these constants that tune different parts of madronalib for space use etc. should all be in one header.
  constexpr int kDefaultSymbolTableSize = 4096;
  
  // very simple hash function from Kernighan & Ritchie.
  // Constexpr version for hashing strings known at compile time.
  template <size_t N>
  constexpr uint32_t krHash2(const char * str)
  {
    return (N > 1) ? ((krHash2<N - 1>(str + 1)) + *str)*31u : 0;
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
    size_t i = len;
    uint32_t accum = 0;
    while(i > 0)
    {
      i--;
      accum += str[i];
      accum *= 31u;
    }
    return accum & kHashTableMask;
  }
  
  inline uint32_t krHash0(const char * str)
  {
    return krHash0(str, strlen(str));
  }

  template<size_t N>
  constexpr uint32_t hash(const char (&sym)[N]) { return krHash1<N>(sym); }

  class HashedCharArray
  {
  public:
    // template ctor from string literals allows hashing for code like Proc::setParam("foo") to be done at compile time.
    template<size_t N>
    constexpr HashedCharArray(const char (&sym)[N]) : len(N), hash(krHash1<N>(sym)), pChars(sym) { }
    
    // this non-constexpr ctor counts the string length at runtime.
    HashedCharArray(const char* pC) : len(strlen(pC)), hash(krHash0(pC, len)), pChars(pC) { }
    
    // this non-constexpr ctor takes a string length parameter at runtime.
    HashedCharArray(const char* pC, size_t lengthBytes) : len(lengthBytes), hash(krHash0(pC, len)), pChars(pC) { }
    
    // default, null ctor
    HashedCharArray() : len(0), hash(0), pChars(nullptr) { }
    
    const size_t len;
    const uint32_t hash;
    const char* pChars;
  };
  
  using SymbolID = size_t;
  
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
    SymbolID getSymbolID(const HashedCharArray& hsl);
    SymbolID getSymbolID(const char * sym);
    SymbolID getSymbolID(const char * sym, size_t lengthBytes);
    
    const TextFragment& getSymbolTextByID(SymbolID symID);
    SymbolID addEntry(const HashedCharArray& hsl);
    
  private:
    
    // vector of text fragments in ID/creation order
    std::vector< TextFragment > mSymbolTextsByID;
    
    // hash table containing indexes to strings for a given hash value.
    struct TableEntry
    {
      std::mutex mMutex;
      std::vector<SymbolID> mIDVector;
    };
    
    void clearEntry(TableEntry& entry)
    {
      std::unique_lock<std::mutex> lock(entry.mMutex);
      entry.mIDVector.clear();
    }
    
    // since the maximum hash value is known, there will be no need to resize this array.
    std::array< TableEntry, kHashTableSize > mHashTable;
    
    size_t mSize{0};
  };
  
  inline SymbolTable& theSymbolTable()
  {
    static const std::unique_ptr<SymbolTable> t (new SymbolTable());
    return *t;
  }
  
  // ----------------------------------------------------------------
#pragma mark Symbol
  
  class Symbol
  {
    // the ID equals the order in which the symbol was created.
    // 2^31 unique symbols are possible. There is no checking for overflow.
    SymbolID id;
    
    friend std::ostream& operator<< (std::ostream& out, const Symbol r);

  public:
    Symbol() : id(0) {}
    Symbol(const HashedCharArray& hsl) : id( theSymbolTable().getSymbolID(hsl) ) { }
    Symbol(const char* pC) : id( theSymbolTable().getSymbolID(pC) ) { }
    Symbol(const char* pC, size_t lengthBytes) : id( theSymbolTable().getSymbolID(pC, lengthBytes) ) { }
    Symbol(TextFragment frag) : id( theSymbolTable().getSymbolID(frag.getText(), frag.lengthInBytes()) ) { } // needed?
    
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
    
    friend uint32_t hash(Symbol s);

    // search hash table for our id to find our hash.
    // for testing only!
    inline int getHashFromTable() const
    {
      int hash = 0;
      
      for(auto& entry : theSymbolTable().mHashTable)
      {
        auto idVec = entry.mIDVector;
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
    inline const TextFragment& getTextFragment() const
    {
      return theSymbolTable().getSymbolTextByID(id);
    }
    
    inline const char* getUTF8Ptr() const
    {
      return theSymbolTable().getSymbolTextByID(id).getText();
    }
    
    SymbolID getID() const { return id; }
    
    inline bool beginsWith(Symbol b) const
    {
      return getTextFragment().beginsWith(b.getTextFragment());
    }
    
    inline bool endsWith(Symbol b) const
    {
      return getTextFragment().endsWith(b.getTextFragment());
    }
    
    // TODO for existing client code, deprecated
    inline std::string toString() const
    {
      return std::string(getUTF8Ptr());
    }
  };
  
  inline uint32_t hash(Symbol f)
  {
    return krHash0(f.getUTF8Ptr());
  }

  inline Symbol operator+(Symbol f1, Symbol f2)
  {
    return Symbol(TextFragment(f1.getTextFragment(), f2.getTextFragment()));
  }
  
  }  // namespace ml

// hashing function for ml::Symbol use in unordered STL containers. simply return the ID,
// which gives each Symbol a unique hash.
namespace std
  {
  template<>
  struct hash<ml::Symbol>
  {
    std::size_t operator()(const ml::Symbol& s) const
    {
      return s.getID();
    }
  };
  }

