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

#include "MLText.h"

namespace ml
{

// hashing: 64-bit FNV-1a

namespace detail
{
constexpr uint64_t fnv1a(uint64_t h, const char* s)
{
  return (*s == 0) ? h :
  fnv1a((h ^ static_cast<uint64_t>(*s)) * 1099511628211ull, s + 1);
}

constexpr uint64_t fnv1aSubstring(uint64_t h, const char* s, size_t len)
{
  return (len == 0) ? h :
  fnv1aSubstring((h ^ static_cast<uint64_t>(*s)) * 1099511628211ull, s + 1, len - 1);
}
}

// FNV-1a hash for null-terminated strings
constexpr uint64_t fnv1a(const char* s)
{
  return detail::fnv1a(14695981039346656037ull, s);
}

// FNV-1a hash for substrings (with length)
constexpr uint64_t fnv1aSubstring(const char* s, size_t len)
{
  return detail::fnv1aSubstring(14695981039346656037ull, s, len);
}

template <size_t N>
constexpr uint64_t hash(const char (&sym)[N])
{
  return fnv1a(sym);
}

// Runtime version for dynamic strings
inline uint64_t fnv1aRuntime(const char* str, size_t len)
{
  uint64_t hash = 14695981039346656037ull;
  for (size_t i = 0; i < len; ++i)
  {
    hash = (hash ^ static_cast<uint64_t>(str[i])) * 1099511628211ull;
  }
  return hash;
}

inline uint64_t fnv1aRuntime(const char* str)
{
  return fnv1aRuntime(str, strlen(str));
}

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
  
  // Constexpr: compute hash only
  template <size_t N>
  constexpr Symbol(const char (&sym)[N]) : mHash(fnv1a(sym)) {}
  
  // Runtime: compute hash AND register
  Symbol(const char* pC)
  : mHash(theSymbolTable().registerSymbol(pC, strlen(pC))) {}
  
  Symbol(const char* pC, size_t lengthBytes)
  : mHash(theSymbolTable().registerSymbol(pC, lengthBytes)) {}
  
  Symbol(TextFragment frag)
  : mHash(theSymbolTable().registerSymbol(frag.getText(), frag.lengthInBytes())) {}
  
  static constexpr Symbol fromHash(uint64_t hash)
  {
    Symbol s;
    s.mHash = hash;
    return s;
  }
  
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

inline Symbol operator+(Symbol f1, Symbol f2)
{
  return Symbol(TextFragment(f1.getTextFragment(), f2.getTextFragment()));
}

inline uint64_t hash(Symbol s) { return s.getHash(); }

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



