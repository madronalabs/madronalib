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
// Symbol stores only a 64-bit hash. All Symbol construction registers the text
// in the SymbolTable. Path uses compile-time hashing for performance without
// requiring Symbol registration.
//
// see also: TextFragment, Path, Tree

#pragma once

#include <cstring>
#include <iostream>
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

// Symbol - stores a 64-bit hash, text looked up in SymbolTable
// All constructors register the symbol in the table

class Symbol
{
  uint64_t mHash;
  friend std::ostream& operator<<(std::ostream& out, const Symbol r);

 public:
  // Default constructor - null symbol
  constexpr Symbol() : mHash(0) {}

  Symbol(const char* pC) : mHash(theSymbolTable().registerSymbol(pC, strlen(pC))) {}

  Symbol(const char* pC, size_t lengthBytes)
      : mHash(theSymbolTable().registerSymbol(pC, lengthBytes))
  {
  }

  explicit Symbol(const TextFragment& frag)
      : mHash(theSymbolTable().registerSymbol(frag.getText(), frag.lengthInBytes()))
  {
  }

  // Comparison
  bool operator<(const Symbol b) const { return mHash < b.mHash; }
  bool operator==(const Symbol b) const { return mHash == b.mHash; }
  bool operator!=(const Symbol b) const { return mHash != b.mHash; }
  explicit operator bool() const { return mHash != 0; }

  // Accessors
  uint64_t getHash() const { return mHash; }

  // Text access - returns "?" if symbol not registered
  const TextFragment& getTextFragment() const { return theSymbolTable().getTextForHash(mHash); }
  const char* getUTF8Ptr() const { return getTextFragment().getText(); }

  // Text operations
  bool beginsWith(Symbol b) const { return getTextFragment().beginsWith(b.getTextFragment()); }
  bool endsWith(Symbol b) const { return getTextFragment().endsWith(b.getTextFragment()); }
  std::string toString() const { return std::string(getUTF8Ptr()); }

  // creates an unregistered Symbol - for use by constexpr Paths only
  explicit constexpr Symbol(uint64_t hash) : mHash(hash) {}
};

// Concatenate two symbols - registers the result
inline Symbol operator+(Symbol f1, Symbol f2)
{
  TextFragment sum(f1.getTextFragment(), f2.getTextFragment());
  return Symbol(sum);
}

inline uint64_t hash(Symbol sym) { return sym.getHash(); }

}  // namespace ml

// hashing function for ml::Symbol use in unordered STL containers
namespace std
{
template <>
struct hash<ml::Symbol>
{
  uint64_t operator()(const ml::Symbol& s) const { return s.getHash(); }
};
}  // namespace std
