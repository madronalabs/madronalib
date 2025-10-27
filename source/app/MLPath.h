// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <numeric>

#include "MLSymbol.h"
#include "MLTextUtils.h"
#include "MLMemoryUtils.h"

namespace ml
{

const int kPathMaxSymbols = 15;

template <class K>
class GenericPath
{
public:
  GenericPath() = default;
  
  explicit operator bool() const { return mSize != 0; }
  
  int getSize() const { return static_cast<int>(mSize); }
  K getElement(int n) const { return _elements[n]; }
  
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
// Template specializations for toText()
// ============================================================================

// Specialization for Symbol paths
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

// Specialization for TextFragment paths
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

class Path final : public GenericPath<Symbol>
{
  friend std::ostream& operator<<(std::ostream& out, const Path& r);
  
public:
  Path() = default;
  
  // Runtime constructors
  Path(const GenericPath<Symbol>& base) : GenericPath<Symbol>(base) {}
  Path(const char* str);
  Path(const Symbol sym) { addElement(sym); }
  Path(const TextFragment frag);
  Path(const TextFragment frag, const char separator);
  Path(const Path& a, const Path& b);
  Path(const Path& a, const Path& b, const Path& c);
  Path(const Path& a, const Path& b, const Path& c, const Path& d);
  
  // Constexpr constructor from string literal - compile-time hashing!
  template <size_t N>
  constexpr Path(const char (&str)[N], char separator = '/')
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
          // Compute hash at compile time and construct Symbol from it
          uint64_t hash = fnv1aSubstring(&str[start], len);
          _elements[mSize++] = Symbol::fromHash(hash);
        }
      }
    }
  }
  
  // Access hashes directly for fast comparison
  uint64_t getHash(int n) const { return _elements[n].getHash(); }
  
private:
  void parsePathString(const char* pathStr, const char delimiter = '/');
};

// Fast hash-based comparison
inline bool operator==(const Path& a, const Path& b)
{
  if (a.getSize() != b.getSize()) return false;
  for (int i = 0; i < a.getSize(); ++i)
  {
    if (a.getHash(i) != b.getHash(i)) return false;
  }
  return true;
}

// Helper functions for Path
inline Symbol head(Path p) { return p.getSize() > 0 ? p.getElement(0) : Symbol(); }
inline Symbol first(Path p) { return head(p); }
inline Symbol second(Path p) { return p.getSize() > 1 ? p.getElement(1) : Symbol(); }
inline Symbol third(Path p) { return p.getSize() > 2 ? p.getElement(2) : Symbol(); }
inline Symbol fourth(Path p) { return p.getSize() > 3 ? p.getElement(3) : Symbol(); }
inline Symbol fifth(Path p) { return p.getSize() > 4 ? p.getElement(4) : Symbol(); }
inline Symbol nth(Path p, size_t n) { return p.getSize() > n ? p.getElement(sizeToInt(n)) : Symbol(); }

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

// Helper for converting Path to text with root separator
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

// ======
/*
inline TextFragment pathToText(Path p, const char separator = '/')
{
  TextFragment r;
  auto n = p.getSize();
  if (n < 1) return r;
  
  r = p.getElement(0).getTextFragment();
  for (int i = 1; i < n; ++i)
  {
    r = TextFragment(r, separator);
    r = TextFragment(r, p.getElement(i).getTextFragment());
  }
  return r;
}
*/

class DynamicPath final : public GenericPath<TextFragment>
{
  friend std::ostream& operator<<(std::ostream& out, const DynamicPath& r);
  
public:
  DynamicPath() = default;
  
  DynamicPath(const GenericPath<TextFragment>& base) : GenericPath<TextFragment>(base) {}
  DynamicPath(const char* str);
  DynamicPath(const TextFragment frag) { addElement(frag); }
  DynamicPath(const TextFragment frag, const char separator);
  DynamicPath(const DynamicPath& a, const DynamicPath& b);
  
private:
  void parsePathString(const char* pathStr, const char delimiter = '/');
};

inline TextFragment head(DynamicPath p) { return p.getSize() > 0 ? p.getElement(0) : TextFragment(); }
inline TextFragment last(DynamicPath p) { return p.getSize() > 0 ? p.getElement(p.getSize() - 1) : TextFragment(); }

} // namespace ml



// Single operator<< for all path types
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




// a Path describes the address of one or more elements in a tree
// of such elements, for example one or more MLProcs in a graph.
//
// the copy number lets the path refer to one specific element in a
// multi-container if the Path ends on that container. Copies are
// indexed starting at 1. If the copy number is 0, the Path refers
// to all the elements.

// TODO in v.2 allow a copy number at each path level
// also consider whether the final symbol (object name or short file name)
// should be a Symbol or rather just a TextFragment

// Maximum path depth allows stack allocation / use in audio threads.


/*


namespace ml
{
class Path final
{
  friend std::ostream& operator<<(std::ostream& out, const Path& r);

 public:
  explicit Path() = default;

  Path(const char* str);
  Path(const Symbol sym);
  Path(const TextFragment frag);

  explicit Path(const TextFragment frag, const char separator);
  explicit Path(const Path& a, const Path& b);
  explicit Path(const Path& a, const Path& b, const Path& c);
  explicit Path(const Path& a, const Path& b, const Path& c, const Path& d);

  ~Path() = default;

  // boolean test.
  explicit operator bool() const { return (mSize != 0); }

  inline int getSize() const { return static_cast<int>(mSize); }
  inline Symbol getElement(int n) const { return _symbols[n]; }
  inline void setElementUnchecked(size_t n, Symbol sym) { _symbols[n] = sym; if(n >= mSize) mSize = n + 1;}
  inline int getCopy() const { return mCopy; }
  inline void setCopy(int c) { mCopy = c; }  // MLTEST to remove, use ctor only?

  bool beginsWith(Path b) const;

  friend class const_iterator;
  class const_iterator
  {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Symbol;
    using difference_type = int;
    using pointer = Symbol*;
    using reference = Symbol&;

    const_iterator(const Path* p) : mpPath(p), mIndex(0) {}
    const_iterator(const Path* p, int startIndex) : mpPath(p), mIndex(startIndex) {}
    ~const_iterator() {}

    bool operator==(const const_iterator& b) const
    {
      if (mpPath != b.mpPath) return false;
      return (mIndex == b.mIndex);
    }

    bool operator!=(const const_iterator& b) const { return !(*this == b); }

    const Symbol operator*() const { return mpPath->getElement(mIndex); }

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

    const_iterator& operator+(int distance)
    {
      for (int n = 0; n < distance; ++n)
      {
        this->operator++();
      }
      return *this;
    }

   private:
    const Path* mpPath;
    int mIndex;
  };

  inline const_iterator begin() const { return const_iterator(this); }
  inline const_iterator end() const { return const_iterator(this, static_cast<int>(mSize)); }

  friend Symbol head(Path p);
  friend Symbol first(Path p);
  friend Symbol second(Path p);
  friend Symbol third(Path p);
  friend Symbol fourth(Path p);
  friend Symbol fifth(Path p);
  friend Symbol nth(Path p, size_t n); // starting from 0th = head
  friend Path tail(Path p);
  friend Path butLast(Path p);
  friend Symbol last(Path p);
  friend Path lastN(Path p, size_t n);

  friend Path substitute(Path p, Symbol from, Symbol to);
  friend Path substitute(Path p, Symbol from, Path to);

 private:
  // TODO should Path be immutable?
  inline void addSymbol(ml::Symbol sym)
  {
    if (mSize < kPathMaxSymbols)
    {
      _symbols[mSize++] = sym;
    }
    else
    {
      // TODO something!
      // //debug() << "Path::addSymbol: max path length exceeded!\n";
    }
  }

  void parsePathString(const char* pathStr, const char delimiter = '/');

  std::array<Symbol, kPathMaxSymbols> _symbols{};
  unsigned char mSize{0};
  unsigned char mCopy{0};
  unsigned char _dummy{0};
  unsigned char _dummy2{0};
  // sizeof(Path) = 64
};

inline bool operator==(const Path& a, const Path& b)
{
  auto an = a.getSize();
  auto bn = b.getSize();
  if (an != bn) return false;
  for (int i = 0; i < an; ++i)
  {
    if (a.getElement(i) != b.getElement(i)) return false;
  }
  return true;
}

inline bool operator!=(const Path a, const Path b) { return !(a == b); }

inline TextFragment pathToText(Path p, const char separator = '/')
{
  TextFragment r;
  auto n = p.getSize();
  if (n < 1) return r;
  r = p.getElement(0).getTextFragment();
  for (int i = 1; i < n; ++i)
  {
    r = TextFragment(r, separator);
    r = TextFragment(r, p.getElement(i).getTextFragment());
  }
  return r;
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

inline Path textToPath(TextFragment t, const char separator = '/')
{
  return Path(t, separator);
}

inline Symbol getExtensionFromPath(Path p)
{
  return Symbol(textUtils::getExtension(last(p).getTextFragment()));
}

inline Path removeExtensionFromPath(Path p)
{
  auto nameWithoutExtension = textUtils::stripExtension(last(p).getTextFragment());
  return Path(butLast(p), Path(nameWithoutExtension));
}

inline Path addExtensionToPath(Path p, TextFragment ext)
{
  auto nameWithoutExtension = last(p).getTextFragment();
  auto nameWithExtension = TextFragment(nameWithoutExtension, ".", ext);
  return Path(butLast(p), Path(nameWithExtension));
}


}  // namespace ml
 */
