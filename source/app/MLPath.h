// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/


// GenericPath
// --------
//
// Uses:
//
// GenericPath is a template class representing a hierarchical path with
// elements of type K. It is the base implementation for both Symbol-based
// paths (compile-time friendly, hash-based) and TextFragment-based paths
// (runtime-only, no symbol table overhead).
//
// Type aliases:
//   Path = GenericPath<Symbol>               // For compile-time trees
//   TextPath = GenericPath<TextFragment>   // For runtime trees
//
// Requirements:
//
// GenericPath are immutable after construction.
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

#include <numeric>

#include "MLSymbol.h"
#include "MLTextUtils.h"
#include "MLMemoryUtils.h"

namespace ml
{

const int kPathMaxSymbols = 15;

// GenericPath - templated on key type

template <class K>
class GenericPath
{
public:
  GenericPath() = default;
  
  // Specialized constructors declared here, defined below via specialization
  GenericPath(const char* str);
  GenericPath(const TextFragment frag, const char separator);
  
  // Constexpr constructor from string literal - only meaningful for Symbol specialization
  template <size_t N>
  constexpr GenericPath(const char (&str)[N], char separator = '/');
  
  GenericPath(const K elem)
  {
    addElement(elem);
  }
  
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
  
  GenericPath(const GenericPath p1, const GenericPath p2, const GenericPath p3, const GenericPath p4)
  {
    for (K elem : p1) addElement(elem);
    for (K elem : p2) addElement(elem);
    for (K elem : p3) addElement(elem);
    for (K elem : p4) addElement(elem);
  }
  
  explicit operator bool() const { return mSize != 0; }
  
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
    
    bool operator==(const const_iterator& b) const { return mpPath == b.mpPath && mIndex == b.mIndex; }
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

// Generic equality
template <class K>
inline bool operator==(const GenericPath<K>& a, const GenericPath<K>& b)
{
  if (a.getSize() != b.getSize()) return false;
  for (int i = 0; i < a.getSize(); ++i)
  {
    if (a.getElement(i) != b.getElement(i)) return false;
  }
  return true;
}

template <class K>
inline bool operator!=(const GenericPath<K>& a, const GenericPath<K>& b)
{
  return !(a == b);
}

// Generic helper functions that work for any GenericPath<K>
template <class K>
inline K head(GenericPath<K> p) { return p.getSize() > 0 ? p.getElement(0) : K(); }

template <class K>
inline K first(GenericPath<K> p) { return head(p); }

template <class K>
inline K second(GenericPath<K> p) { return p.getSize() > 1 ? p.getElement(1) : K(); }

template <class K>
inline K third(GenericPath<K> p) { return p.getSize() > 2 ? p.getElement(2) : K(); }

template <class K>
inline K fourth(GenericPath<K> p) { return p.getSize() > 3 ? p.getElement(3) : K(); }

template <class K>
inline K fifth(GenericPath<K> p) { return p.getSize() > 4 ? p.getElement(4) : K(); }

template <class K>
inline K nth(GenericPath<K> p, size_t n) { return p.getSize() > n ? p.getElement(n) : K(); }

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
    for(size_t i = len - n; i < len; ++i)
    {
      r = GenericPath<K>(r, nth(p, i));
    }
    return r;
  }
  return GenericPath<K>();
}


// Path
// --------
//
// Uses:
//
// Path represents a hierarchical address in a tree structure, such as
// "/audio/oscillator/frequency". Paths are used as keys in Tree containers
// and for addressing elements in nested data structures.
//
// Path is a type alias for GenericPath<Symbol>, making it hash-based and
// compile-time friendly. For runtime-constructed paths (like file systems),
// use TextPath (GenericPath<TextFragment>).
//
// Requirements:
//
// Paths are immutable after construction.
//
// Paths constructed from string literals at compile time (constexpr) have
// zero runtime initialization cost. The path segments are hashed at compile
// time using FNV-1a, and no symbol table modification occurs until the first
// string access.
//
// Accessing a Path created at runtime causes no heap allocation if all its
// constituent Symbols already exist in the symbol table. This allows use in
// DSP code, assuming the signal graph has already been parsed.
//
// Path comparison is extremely fast, using hash comparison rather than string
// comparison.
//
// Maximum path depth is 15 segments (kPathMaxSymbols).
//
// see also: Symbol, GenericPath, TextPath, Tree

using Path = GenericPath<Symbol>;

// Specialized constructors for Path (GenericPath<Symbol>)

// Helper function for parsing path strings into Symbols using UTF library
void parsePathStringIntoSymbols(Path& path, const char* pathStr, const char delimiter = '/');

template <>
inline Path::GenericPath(const char* str)
{
  parsePathStringIntoSymbols(*this, str, '/');
}

template <>
inline Path::GenericPath(const TextFragment frag, const char separator)
{
  parsePathStringIntoSymbols(*this, frag.getText(), separator);
}

template <>
template <size_t N>
constexpr Path::GenericPath(const char (&str)[N], char separator)
: mSize(0), mCopy(0)
{
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
        uint64_t hash = fnv1aSubstring(&str[start], len);
        _elements[mSize++] = Symbol::fromHash(hash);
      }
    }
  }
}

// Extension method for Symbol paths

inline uint64_t getHash(const Path& p, int n)
{
  return p.getElement(n).getHash();
}

// Fast hash-based equality for Symbol paths

template <>
inline bool operator==(const Path& a, const Path& b)
{
  if (a.getSize() != b.getSize()) return false;
  for (int i = 0; i < a.getSize(); ++i)
  {
    if (getHash(a, i) != getHash(b, i)) return false;
  }
  return true;
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
// Unlike SymbolPath, TextPath stores TextFragments directly
// rather than hash references, avoiding symbol table overhead and registration
// cost for transient or one-time-use paths.
//
// Requirements:
//
// DynamicPaths are immutable after construction.
//
// DynamicPaths do not interact with the symbol table at all - no registration,
// no hash computation, no collision detection. This makes them suitable for
// temporary paths that don't need the performance benefits of Symbol hashing.
//
// Memory allocation depends on TextFragment's small string optimization. Paths
// with segments under 16 bytes require no heap allocation.
//
// see also: Path, GenericPath, TextFragment, Tree, TextTree

using TextPath = GenericPath<TextFragment>;


// Helper function for parsing path strings into TextFragments using UTF library
void parsePathStringIntoTextFragments(TextPath& path, const char* pathStr, const char delimiter = '/');

template <>
inline GenericPath<TextFragment>::GenericPath(const char* str)
{
  parsePathStringIntoTextFragments(*this, str, '/');
}

template <>
inline GenericPath<TextFragment>::GenericPath(const TextFragment frag, const char separator)
{
  parsePathStringIntoTextFragments(*this, frag.getText(), separator);
}


// Path-specific methods (Symbol specialization)

// Add getHash() method for Symbol paths only

namespace detail
{
template <class K>
struct PathHashAccessor
{
  // No getHash for generic paths
};

template <>
struct PathHashAccessor<Symbol>
{
  static uint64_t getHash(const Path& p, int n)
  {
    return p.getElement(n).getHash();
  }
};
}

// Template specializations for toText()

template <>
inline TextFragment Path::toText(const char separator) const
{
  TextFragment r;
  auto n = getSize();
  if (n < 1) return r;
  
  r = getElement(0).getTextFragment();
  for (int i = 1; i < n; ++i)
  {
    r = TextFragment(r, separator);
    r = TextFragment(r, getElement(i).getTextFragment());
  }
  return r;
}

template <>
inline TextFragment GenericPath<TextFragment>::toText(const char separator) const
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

inline Path substitute(Path p, Symbol fromSymbol, Path toPath)
{
  Path r;
  for (int n = 0; n < p.getSize(); ++n)
  {
    Symbol next = p.getElement(n);
    if (next == fromSymbol)
    {
      r = Path{r, toPath};
    }
    else
    {
      r = Path{r, Path(next.getTextFragment())};
    }
  }
  return r;
}

 
// operator<<

inline std::ostream& operator<<(std::ostream& out, const ml::Path & r)
{
  out << r.toText();
  return out;
}

inline std::ostream& operator<<(std::ostream& out, const ml::TextPath & r)
{
  out << r.toText();
  return out;
}



}  // namespace ml
