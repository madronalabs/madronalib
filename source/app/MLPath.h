// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <numeric>

#include "MLSymbol.h"
#include "MLTextUtils.h"
#include "MLMemoryUtils.h"

#include "utf.hpp"

namespace ml
{

const int kPathMaxSymbols = 15;

// ============================================================================
// GenericPath - templated on key type
// ============================================================================

template <class K>
class GenericPath

public:
  GenericPath() = default;
  
  // Specialized constructors declared here, defined below via specialization
  GenericPath(const char* str);
  GenericPath(const TextFragment frag);
  GenericPath(const TextFragment frag, const char separator);
  
  // Constexpr constructor from string literal - only meaningful for Symbol specialization
  template <size_t N>
  constexpr GenericPath(const char (&str)[N], char separator = '/');
  
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
  
  int getCopy() const { return mCopy; }
  void setCopy(int c) { mCopy = c; }
  
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

// ============================================================================
// Type aliases - Path and DynamicPath are just specialized GenericPaths
// ============================================================================

using Path = GenericPath<Symbol>;
using DynamicPath = GenericPath<TextFragment>;

// ============================================================================
// Specialized constructors for Path (GenericPath<Symbol>)
// ============================================================================

// Helper function for parsing path strings into Symbols
inline void parsePathStringIntoSymbols(GenericPath<Symbol>& path, const char* pathStr, const char delimiter = '/')
{
  if (!pathStr) return;
  
  auto it = TextFragment::Iterator(pathStr);
  const char* symbolStartPtr = pathStr;
  
  do
  {
    size_t symbolSizeInBytes = 0;
    
    // Skip separators
    while (*it == delimiter)
    {
      symbolStartPtr++;
      ++it;
    }
    
    // Advance to end of symbol
    while ((*it != delimiter) && (*it != '\0'))
    {
      symbolSizeInBytes += utf::internal::utf_traits<utf::utf8>::write_length(*it);
      ++it;
    }
    
    // Create and add the symbol
    if (symbolSizeInBytes > 0)
    {
      path.addElement(Symbol(symbolStartPtr, symbolSizeInBytes));
      symbolStartPtr += symbolSizeInBytes;
    }
  } while (*it != '\0');
}

template <>
inline GenericPath<Symbol>::GenericPath(const char* str)
{
  parsePathStringIntoSymbols(*this, str, '/');
}

template <>
inline GenericPath<Symbol>::GenericPath(const TextFragment frag)
{
  parsePathStringIntoSymbols(*this, frag.getText(), '/');
}

template <>
inline GenericPath<Symbol>::GenericPath(const TextFragment frag, const char separator)
{
  parsePathStringIntoSymbols(*this, frag.getText(), separator);
}

template <>
template <size_t N>
constexpr GenericPath<Symbol>::GenericPath(const char (&str)[N], char separator)
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

/*
// Additional constructors for combining paths
template <>
inline GenericPath<Symbol>::GenericPath(const Path& a, const Path& b)
{
  for (Symbol s : a) addElement(s);
  for (Symbol s : b) addElement(s);
}

template <>
inline GenericPath<Symbol>::GenericPath(const Path& a, const Path& b, const Path& c)
{
  for (Symbol s : a) addElement(s);
  for (Symbol s : b) addElement(s);
  for (Symbol s : c) addElement(s);
}

template <>
inline GenericPath<Symbol>::GenericPath(const Path& a, const Path& b, const Path& c, const Path& d)
{
  for (Symbol s : a) addElement(s);
  for (Symbol s : b) addElement(s);
  for (Symbol s : c) addElement(s);
  for (Symbol s : d) addElement(s);
}

// Single-symbol constructor
template <>
inline GenericPath<Symbol>::GenericPath(const Symbol sym)
{
  addElement(sym);
}
*/

// ============================================================================
// Specialized constructors for DynamicPath (GenericPath<TextFragment>)
// ============================================================================

inline void parsePathStringIntoTextFragments(GenericPath<TextFragment>& path, const char* pathStr, const char delimiter = '/')
{
  if (!pathStr) return;
  
  auto it = TextFragment::Iterator(pathStr);
  const char* symbolStartPtr = pathStr;
  
  do
  {
    size_t symbolSizeInBytes = 0;
    
    // Skip separators
    while (*it == delimiter)
    {
      symbolStartPtr++;
      ++it;
    }
    
    // Advance to end of symbol
    while ((*it != delimiter) && (*it != '\0'))
    {
      symbolSizeInBytes += utf::internal::utf_traits<utf::utf8>::write_length(*it);
      ++it;
    }
    
    // Create and add the TextFragment
    if (symbolSizeInBytes > 0)
    {
      path.addElement(TextFragment(symbolStartPtr, static_cast<int>(symbolSizeInBytes)));
      symbolStartPtr += symbolSizeInBytes;
    }
  } while (*it != '\0');
}

template <>
inline GenericPath<TextFragment>::GenericPath(const char* str)
{
  parsePathStringIntoTextFragments(*this, str, '/');
}

template <>
inline GenericPath<TextFragment>::GenericPath(const TextFragment frag)
{
  parsePathStringIntoTextFragments(*this, frag.getText(), '/');
}

template <>
inline GenericPath<TextFragment>::GenericPath(const TextFragment frag, const char separator)
{
  parsePathStringIntoTextFragments(*this, frag.getText(), separator);
}

/*
template <>
inline GenericPath<TextFragment>::GenericPath(const DynamicPath& a, const DynamicPath& b)
{
  for (TextFragment f : a) addElement(f);
  for (TextFragment f : b) addElement(f);
}
*/

// ============================================================================
// Path-specific methods (Symbol specialization)
// ============================================================================

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
  static uint64_t getHash(const GenericPath<Symbol>& p, int n)
  {
    return p.getElement(n).getHash();
  }
};
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

// ============================================================================
// Template specializations for toText()
// ============================================================================

template <>
inline TextFragment GenericPath<Symbol>::toText(const char separator) const
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

} // namespace ml

// ============================================================================
// operator<< in global namespace for ADL
// ============================================================================

template <class K>
inline std::ostream& operator<<(std::ostream& out, const ml::GenericPath<K>& r)
{
  out << r.toText();
  unsigned copy = r.getCopy();
  if (copy)
  {
    out << "(#" << copy << ")";
  }
  return out;
}

namespace ml
{

// ============================================================================
// Helper functions for Path
// ============================================================================

inline Symbol head(Path p) { return p.getSize() > 0 ? p.getElement(0) : Symbol(); }
inline Symbol first(Path p) { return head(p); }
inline Symbol second(Path p) { return p.getSize() > 1 ? p.getElement(1) : Symbol(); }
inline Symbol third(Path p) { return p.getSize() > 2 ? p.getElement(2) : Symbol(); }
inline Symbol fourth(Path p) { return p.getSize() > 3 ? p.getElement(3) : Symbol(); }
inline Symbol fifth(Path p) { return p.getSize() > 4 ? p.getElement(4) : Symbol(); }
inline Symbol nth(Path p, size_t n) { return p.getSize() > n ? p.getElement(n) : Symbol(); }

inline Path tail(Path p)
{
  Path r;
  r.setCopy(p.getCopy());
  for (int n = 1; n < p.getSize(); ++n)
  {
    r.addElement(p.getElement(n));
  }
  return r;
}

inline Path butLast(Path p)
{
  Path r;
  for (int n = 0; n < p.getSize() - 1; ++n)
  {
    r.addElement(p.getElement(n));
  }
  return r;
}

inline Symbol last(Path p)
{
  return p.getSize() > 0 ? p.getElement(p.getSize() - 1) : Symbol();
}

inline TextFragment rootPathToText(Path p, const char separator = '/')
{
  TextFragment r;
  auto n = p.getSize();
  for (int i = 0; i < n; ++i)
  {
    r = TextFragment(r, separator);
    r = TextFragment(r, p.getElement(i).getTextFragment());
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

/*
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
      r = Path{r, Path(next)};
    }
  }
  return r;
}
*/

// Helper functions for DynamicPath
inline TextFragment head(DynamicPath p) { return p.getSize() > 0 ? p.getElement(0) : TextFragment(); }
inline TextFragment last(DynamicPath p) { return p.getSize() > 0 ? p.getElement(p.getSize() - 1) : TextFragment(); }

}  // namespace ml
 
