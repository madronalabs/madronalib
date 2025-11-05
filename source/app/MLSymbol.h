// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// Symbol
// --------
//
// Uses:
//
// Symbol is designed to be an efficient key in STL containers such as map
// and unordered_map, that is quick to convert to and from a unique UTF-8
// string.
//
// Requirements:
//
// Symbols are immutable.
//
// Symbols must not ever require any heap as long as they are smaller than a
// certain size. Currently this relies on the "small string optimization"
// implementation of TextFragment. Currently the size is 16 bytes.
//
// Accessing a Symbol, no matter the size, must not cause any heap to be allocated
// if the symbol already exists. This allows use in DSP code, assuming that any
// code that sets up signal-keyed structures has already been parsed.
//
// see also: TextFragment, Path, Tree

#pragma once

#include <array>
#include <cstring>
#include <iostream>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "MLHash.h"
#include "MLText.h"

namespace ml
{

// SymbolTable: stores symbol texts by their hashes.

class SymbolTable
{
public:

  SymbolTable() = default;
  ~SymbolTable() = default;
  
  // modifiers
  uint64_t registerSymbol(const char* text, size_t len);
  void clear();
  
  // accessors
  const TextFragment& getTextForHash(uint64_t hash) const;
  size_t getSize() const { return mSymbols.size(); }

  // utilities
  void dump();
  
private:
  std::unordered_map<uint64_t, TextFragment> mSymbols;
  mutable std::mutex mMutex;
  static const TextFragment kNullText;
};

inline SymbolTable& theSymbolTable()
{
  static std::unique_ptr<SymbolTable> t(new SymbolTable());
  return *t;
}


// Symbol itself

class Symbol
{
  uint64_t mHash;
  friend std::ostream& operator<<(std::ostream& out, const Symbol r);
  
public:
  constexpr Symbol() : mHash(0) {}
  explicit constexpr Symbol(uint64_t hash) : mHash(hash) {}

  // Constexpr when possible: compute hash only
  template <size_t N>
  inline constexpr Symbol(const char (&sym)[N]) : mHash(fnv1aSubstring(sym, N - 1)) { }

  //explicit Symbol(const char* pChars, size_t len) noexcept;

  
  /*
  // Runtime: compute hash AND register
  Symbol(const char* pC)
  : mHash(theSymbolTable().registerSymbol(pC, strlen(pC))) {}
  
  Symbol(const char* pC, size_t lengthBytes)
  : mHash(theSymbolTable().registerSymbol(pC, lengthBytes)) {}
  
  explicit Symbol(TextFragment frag)
  : mHash(theSymbolTable().registerSymbol(frag.getText(), frag.lengthInBytes())) {}
  
  static constexpr Symbol fromHash(uint64_t hash)
  {
    Symbol s;
    s.mHash = hash;
    return s;
  }
  */
  
  bool operator<(const Symbol b) const { return mHash < b.mHash; }
  bool operator==(const Symbol b) const { return mHash == b.mHash; }
  bool operator!=(const Symbol b) const { return mHash != b.mHash; }
  explicit operator bool() const { return mHash != 0; }
  
  friend uint64_t hash(Symbol s);
  
  uint64_t getHash() const { return mHash; }
  const TextFragment& getTextFragment() const { return theSymbolTable().getTextForHash(mHash); }
  const char* getUTF8Ptr() const { return getTextFragment().getText(); }
  
  bool beginsWith(Symbol b) const { return getTextFragment().beginsWith(b.getTextFragment()); }
  bool endsWith(Symbol b) const { return getTextFragment().endsWith(b.getTextFragment()); }
  std::string toString() const { return std::string(getUTF8Ptr()); }
};

inline Symbol runtimeSymbol(const char* str)
{
  return Symbol(theSymbolTable().registerSymbol(str, strlen(str)));
}

inline Symbol runtimeSymbol(const char* str, size_t len)
{
  return Symbol(theSymbolTable().registerSymbol(str, len));
}

inline Symbol runtimeSymbol(const TextFragment& frag)
{
  const char* str = frag.getText();
  return Symbol(theSymbolTable().registerSymbol(str, strlen(str)));
}

inline Symbol operator+(Symbol f1, Symbol f2)
{
  TextFragment sum(f1.getTextFragment(), f2.getTextFragment());
  return runtimeSymbol(sum.getText());
}

inline uint64_t hash(Symbol sym) { return sym.getHash(); }

}  // namespace ml


// hashing function for ml::Symbol use in unordered STL containers. 
namespace std
{
template <>
struct hash<ml::Symbol>
{
  uint64_t operator()(const ml::Symbol& s) const { return s.getHash(); }
};

}  // namespace std



