// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// GenericPath
// --------
//
// Uses:
//
// GenericPath is a template class representing a hierarchical path with
// elements of type K. It is the base implementation for both hash-based
// paths (compile-time friendly) and TextFragment-based paths
// (runtime-only, no symbol table overhead).
//
// Type aliases:
//   SymbolHash = uint64_t                     // Hash type for extensibility
//   Path = GenericPath<SymbolHash>            // For compile-time trees
//   TextPath = GenericPath<TextFragment>      // For runtime trees
//
// Requirements:
//
// Paths are immutable after construction.
//
// The maximum path depth is fixed at compile time (kPathMaxSymbols = 15),
// allowing stack allocation and use in real-time audio threads.
//
// GenericPath provides a common interface for all path types via template
// specialization. Type-specific behavior (construction from strings, text
// conversion, compile-time hashing) is implemented through specialization
// rather than inheritance.
//
// Path elements can be accessed by index, iterated over, and converted to
// text representation. Comparison and equality operations are supported.
//
// see also: Symbol, TextFragment, Path, TextPath, Tree

#pragma once

#include <array>
#include <iostream>
#include <vector>

#include "MLHash.h"
#include "MLSymbol.h"
#include "MLTextUtils.h"

namespace ml
{

const int kPathMaxSymbols = 15;

// Type alias for hash-based paths - allows future extensibility
using SymbolHash = uint64_t;


// GenericPath - templated on key type


template <class K>
class GenericPath
{
 public:
  constexpr GenericPath() = default;

  /*
  // Specialized constructors declared here, defined below via specialization
  template <size_t N>
  constexpr GenericPath(const char (&str)[N]);
*/
  
  GenericPath(const char* str);
  GenericPath(const TextFragment& frag);

  // Combining paths
  GenericPath(const GenericPath p1, const GenericPath p2)
  {
    for (K elem : p1) addElement(elem);
    for (K elem : p2) addElement(elem);
  }

  GenericPath(const GenericPath p1, const GenericPath p2, const GenericPath p3)
  {
    for (K elem : p1) addElement(elem);
    for (K elem : p2) addElement(elem);
    for (K elem : p3) addElement(elem);
  }

  GenericPath(const GenericPath p1, const GenericPath p2, const GenericPath p3,
              const GenericPath p4)
  {
    for (K elem : p1) addElement(elem);
    for (K elem : p2) addElement(elem);
    for (K elem : p3) addElement(elem);
    for (K elem : p4) addElement(elem);
  }

  // Comparison
  bool operator==(const GenericPath b) const
  {
    if (getSize() != b.getSize()) return false;
    for (int i = 0; i < getSize(); ++i)
    {
      if (getElement(i) != b.getElement(i)) return false;
    }
    return true;
  }

  bool operator!=(const GenericPath b) const { return !(operator==(b)); }
  explicit operator bool() const { return mSize != 0; }

  // Accessors
  int getSize() const { return static_cast<int>(mSize); }
  K getElement(size_t n) const { return _elements[n]; }

  void setElement(size_t n, K elem)
  {
    if (n < kPathMaxSymbols)
    {
      _elements[n] = elem;
      if (n >= mSize) mSize = n + 1;
    }
  }

  bool beginsWith(GenericPath<K> b) const
  {
    if (b.getSize() > getSize()) return false;
    for (int i = 0; i < b.getSize(); ++i)
    {
      if (_elements[i] != b._elements[i]) return false;
    }
    return true;
  }

  // Convert path to text - specialized for each key type
  TextFragment toText(const char separator = '/') const;

  // Iterator
  class const_iterator
  {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = K;
    using difference_type = int;
    using pointer = K*;
    using reference = K&;

    const_iterator(const GenericPath<K>* p) : mpPath(p), mIndex(0) {}
    const_iterator(const GenericPath<K>* p, int startIndex) : mpPath(p), mIndex(startIndex) {}

    bool operator==(const const_iterator& b) const
    {
      return mpPath == b.mpPath && mIndex == b.mIndex;
    }
    bool operator!=(const const_iterator& b) const { return !(*this == b); }
    K operator*() const { return mpPath->getElement(mIndex); }

    const const_iterator& operator++()
    {
      mIndex++;
      return *this;
    }

    const_iterator& operator++(int)
    {
      this->operator++();
      return *this;
    }

   private:
    const GenericPath<K>* mpPath;
    int mIndex;
  };

  const_iterator begin() const { return const_iterator(this); }
  const_iterator end() const { return const_iterator(this, static_cast<int>(mSize)); }

  void addElement(K elem)
  {
    if (mSize < kPathMaxSymbols)
    {
      _elements[mSize++] = elem;
    }
  }

 protected:
  std::array<K, kPathMaxSymbols> _elements{};
  unsigned char mSize{0};
  unsigned char mCopy{0};
};


// Generic helper functions that work for any GenericPath<K>


template <class K>
inline K head(GenericPath<K> p)
{
  return p.getSize() > 0 ? p.getElement(0) : K();
}

template <class K>
inline K first(GenericPath<K> p)
{
  return head(p);
}

template <class K>
inline K second(GenericPath<K> p)
{
  return p.getSize() > 1 ? p.getElement(1) : K();
}

template <class K>
inline K third(GenericPath<K> p)
{
  return p.getSize() > 2 ? p.getElement(2) : K();
}

template <class K>
inline K fourth(GenericPath<K> p)
{
  return p.getSize() > 3 ? p.getElement(3) : K();
}

template <class K>
inline K fifth(GenericPath<K> p)
{
  return p.getSize() > 4 ? p.getElement(4) : K();
}

template <class K>
inline K nth(GenericPath<K> p, size_t n)
{
  return p.getSize() > n ? p.getElement(n) : K();
}

template <class K>
inline GenericPath<K> tail(GenericPath<K> p)
{
  GenericPath<K> r;
  for (int n = 1; n < p.getSize(); ++n)
  {
    r.addElement(p.getElement(n));
  }
  return r;
}

template <class K>
inline GenericPath<K> butLast(GenericPath<K> p)
{
  GenericPath<K> r;
  for (int n = 0; n < p.getSize() - 1; ++n)
  {
    r.addElement(p.getElement(n));
  }
  return r;
}

template <class K>
inline K last(GenericPath<K> p)
{
  return p.getSize() > 0 ? p.getElement(p.getSize() - 1) : K();
}

template <class K>
inline GenericPath<K> lastN(GenericPath<K> p, size_t n)
{
  auto len = p.getSize();
  if (len >= n)
  {
    GenericPath<K> r;
    for (size_t i = len - n; i < len; ++i)
    {
      r.addElement(p.getElement(i));
    }
    return r;
  }
  return GenericPath<K>();
}


// Path - hash-based, compile-time friendly


// Path
// --------
//
// Uses:
//
// Path represents a hierarchical address in a tree structure, such as
// "/audio/oscillator/frequency". Paths are used as keys in Tree containers
// and for addressing elements in nested data structures.
//
// Path is a type alias for GenericPath<Symbol>, storing 64-bit hashes
// computed from path segment strings. Paths are compile-time friendly - hashes
// are computed at compile-time for string literals, with zero runtime cost.
//
// For runtime-constructed paths (like file systems), use TextPath
// (GenericPath<TextFragment>).
//
// Requirements:
//
// Paths are immutable after construction.
//
// Paths constructed from string literals at compile time (constexpr) have
// zero runtime initialization cost. The path segments are hashed at compile
// time using FNV-1a. Symbol registration (for text lookup) happens separately,
// typically when the Tree is populated or via runtimePath()/PathList.
//
// Path comparison is extremely fast, using hash comparison rather than string
// comparison.
//
// Path stores only hashes. To get the text representation, Symbols must be
// registered in the SymbolTable. This happens automatically via runtimePath(),
// PathList, or Tree operations.
//
// Maximum path depth is 15 segments (kPathMaxSymbols).
//
// see also: Symbol, GenericPath, TextPath, Tree, runtimePath, PathList

using Path = GenericPath<Symbol>;

/*
// Constexpr constructor for Path - computes hashes at compile-time, no Symbol registration
template <>
template <size_t N>
constexpr Path::GenericPath(const char (&str)[N]) : _elements{}, mSize(0), mCopy(0)
{
  const char separator = '/';
  size_t pos = 0;
  while (pos < N - 1 && str[pos] != '\0' && mSize < kPathMaxSymbols)
  {
    // Skip separators
    while (pos < N - 1 && str[pos] == separator) ++pos;

    // Hash segment
    if (pos < N - 1 && str[pos] != '\0')
    {
      size_t start = pos;
      size_t len = 0;
      while (pos < N - 1 && str[pos] != separator && str[pos] != '\0')
      {
        ++len;
        ++pos;
      }

      if (len > 0)
      {
        _elements[mSize++] = Symbol(fnv1aSubstring(&str[start], len));
      }
    }
  }
}
*/

// Runtime path creation with symbol registration
// Use this when you need symbols to be registered (for printing, debugging, etc.)
Path runtimePath(const char* str);
Path runtimePath(const Symbol& sym);
Path runtimePath(const TextFragment& frag);

template <>
inline Path::GenericPath(const char* str)
{
  *this = runtimePath(str);
}

template <>
inline Path::GenericPath(const TextFragment& frag)
{
  *this = runtimePath(frag);
}

// GenericPath<Symbol>-specific helper functions
inline Path substitute(Path p, Symbol from, Symbol to)
{
  Path r{p};
  for (int n = 0; n < p.getSize(); ++n)
  {
    if (p.getElement(n) == from)
    {
      r.setElement(n, to);
    }
  }
  return r;
}

inline Path substitute(Path p, Symbol fromSym, Path toPath)
{
  Path r;
  for (int n = 0; n < p.getSize(); ++n)
  {
    Symbol nextSym = p.getElement(n);
    if (nextSym == fromSym)
    {
      r = Path{r, toPath};
    }
    else
    {
      Path singleElement;
      singleElement.addElement(nextSym);
      r = Path{r, singleElement};
    }
  }
  return r;
}


// TextPath
// --------
//
// Uses:
//
// TextPath is a type alias for GenericPath<TextFragment>, designed for
// runtime-constructed hierarchical paths where the structure is not known at
// compile time. Typical uses include file system paths, user input, and
// dynamically generated content.
//
// Unlike Path, TextPath stores TextFragments directly rather than hash
// references, avoiding symbol table overhead and registration cost for
// transient or one-time-use paths.
//
// Requirements:
//
// TextPaths are immutable after construction.
//
// TextPaths do not interact with the symbol table at all - no registration,
// no hash computation, no collision detection. This makes them suitable for
// temporary paths that don't need the performance benefits of hash-based
// comparison.
//
// Memory allocation depends on TextFragment's small string optimization. Paths
// with segments under 16 bytes require no heap allocation.
//
// see also: Path, GenericPath, TextFragment, Tree

using TextPath = GenericPath<TextFragment>;

/*
// Constexpr constructor for TextPath
template <>
template <size_t N>
constexpr TextPath::GenericPath(const char (&str)[N]) : _elements{}, mSize(0), mCopy(0)
{
  const char separator = '/';
  size_t pos = 0;
  while (pos < N - 1 && str[pos] != '\0' && mSize < kPathMaxSymbols)
  {
    // Skip separators
    while (pos < N - 1 && str[pos] == separator) ++pos;

    // Add segment as TextFragment
    if (pos < N - 1 && str[pos] != '\0')
    {
      size_t start = pos;
      size_t len = 0;
      while (pos < N - 1 && str[pos] != separator && str[pos] != '\0')
      {
        ++len;
        ++pos;
      }

      if (len > 0)
      {
        _elements[mSize++] = TextFragment(&str[start], len);
      }
    }
  }
}
*/

// Runtime constructor for TextPath from TextFragment
// Runtime path creation with symbol registration
// Use this when you need symbols to be registered (for printing, debugging, etc.)
TextPath runtimeTextPath(const char* str, const char separator = '/');
TextPath runtimeTextPath(const Symbol& sym);
TextPath runtimeTextPath(const TextFragment& frag);

template <>
inline TextPath::GenericPath(const TextFragment& frag)
{
  *this = runtimeTextPath(frag.getText());
}

template <>
inline TextPath::GenericPath(const char* str)
{
  *this = runtimeTextPath(str);
}


// Template specializations for toText()


template <>
inline TextFragment Path::toText(const char separator) const
{
  TextFragment r;
  auto n = getSize();
  if (n < 1) return r;

  // Convert hash to Symbol and get text
  Symbol s0(getElement(0));
  r = s0.getTextFragment();

  for (int i = 1; i < n; ++i)
  {
    r = TextFragment(r, separator);
    Symbol s(getElement(i));
    r = TextFragment(r, s.getTextFragment());
  }
  return r;
}

template <>
inline TextFragment TextPath::toText(const char separator) const
{
  TextFragment r;
  auto n = getSize();
  if (n < 1) return r;

  r = getElement(0);
  for (int i = 1; i < n; ++i)
  {
    r = TextFragment(r, separator);
    r = TextFragment(r, getElement(i));
  }
  return r;
}


// Stream operators


inline std::ostream& operator<<(std::ostream& out, const Path& r)
{
  out << r.toText();
  return out;
}

inline std::ostream& operator<<(std::ostream& out, const TextPath& r)
{
  out << r.toText();
  return out;
}


// Convenience functions


inline Path textToPath(const TextFragment& t) { return runtimePath(t); }
inline TextFragment pathToText(const Path& p) { return p.toText(); }
inline TextFragment pathToText(const TextPath& p) { return p.toText(); }

class PathList
{
 public:
  PathList(std::initializer_list<const char*> paths)
  {
    _paths.reserve(paths.size());
    for (const char* p : paths)
    {
      _paths.emplace_back(p);  // Runtime construction, registers symbols
    }
  }

  // Iterator support for range-based for
  auto begin() const { return _paths.begin(); }
  auto end() const { return _paths.end(); }

  size_t size() const { return _paths.size(); }
  const Path& operator[](size_t i) const { return _paths[i]; }

 private:
  std::vector<Path> _paths;
};

}  // namespace ml
