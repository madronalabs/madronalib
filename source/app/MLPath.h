
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <numeric>

#include "MLSymbol.h"
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

const int kPathMaxSymbols = 15;

namespace ml
{
class Path final
{
  friend std::ostream& operator<<(std::ostream& out, const Path& r);

 public:
  Path() = default;
  Path(const Path& b) = default;
  Path(const char* str);
  Path(const Symbol sym);
  Path(const TextFragment frag);
  Path(const TextFragment frag, const char separator);
  Path(const Path& a, const Path& b);

  ~Path() = default;

  // boolean test.
  explicit operator bool() const { return (mSize != 0); }

  // TODO Path should be immutable?
  void addSymbol(Symbol sym);

  inline int getSize() const { return static_cast<int>(mSize); }
  inline Symbol getElement(int n) const { return _symbols[n]; }
  inline int getCopy() const { return mCopy; }
  inline void setCopy(int c) { mCopy = c; }  // MLTEST to remove, use ctor only?

  friend class const_iterator;
  class const_iterator
      : public std::iterator<std::forward_iterator_tag, ml::Symbol>
  {
   public:
    const_iterator(const Path* p) : mpPath(p), mIndex(0) {}
    const_iterator(const Path* p, int startIndex)
        : mpPath(p), mIndex(startIndex)
    {
    }
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

  inline const_iterator end() const
  {
    return const_iterator(this, static_cast<int>(mSize));
  }

  friend Path concat(const Path& a, const Path& b);

  friend Symbol head(Path p);
  friend Path tail(Path p);
  friend Path butLast(Path p);
  friend Symbol last(Path p);

 private:
  void parsePathString(const char* pathStr, const char delimiter = '/');

  std::array<Symbol, kPathMaxSymbols> _symbols{};
  unsigned char mSize{0};
  unsigned char mCopy{0};
  unsigned char _dummy{0};
  unsigned char _dummy2{0};
  // sizeof(Path) = 64
};

inline bool operator==(const Path a, const Path b)
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

inline bool operator!=(Path a, Path b) { return !(a == b); }

}  // namespace ml
