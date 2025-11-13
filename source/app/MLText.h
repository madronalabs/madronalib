// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// assignable but otherwise immutable UTF-8 text object class

#pragma once

#include <memory>
#include <vector>

#include "MLHash.h"

namespace ml
{
static constexpr int kShortFragmentSizeInCodePoints = 16;
static constexpr int kShortFragmentSizeInChars = kShortFragmentSizeInCodePoints * 4;

using CodePoint = char32_t;

// TextFragment: a string class designed to avoid using the heap. Guaranteed not to allocate
// heap if the length in bytes is below kShortFragmentSize.

class TextFragment
{
 public:
  class Iterator
  {
    class Impl;
    std::unique_ptr<Impl> pImpl;

   public:
    Iterator(const char* pos);

    ~Iterator();  // defined in the implementation file, where impl is a
                  // complete type

    Iterator(const Iterator&);
    Iterator& operator=(const Iterator&) = delete;

    CodePoint operator*();
    Iterator& operator++();
    CodePoint operator++(int i);

    friend bool operator!=(Iterator lhs, Iterator rhs);
    friend bool operator==(Iterator lhs, Iterator rhs);
  };

  TextFragment() noexcept;

  /*
   this could be a good idea but the (const char*) ctor is taking precedence, revisit
   template<size_t N> TextFragment(const char(&p)[N]) noexcept : _size(N)
   {
     std::cout << "?";
     _allocate();
     if(_pText)
     {
     std::copy(p, p + _size, _pText);
     _nullTerminate();
     }
   }
   */

  TextFragment(const char* pChars) noexcept;

  explicit TextFragment(const char* pChars, size_t len) noexcept;

  // single code point ctor
  TextFragment(CodePoint c) noexcept;

  // copy ctor
  TextFragment(const TextFragment& a) noexcept;

  // copy assignment operator: TextFragment is assignable but otherwise immutable.
  TextFragment& operator=(const TextFragment& b) noexcept;

  // move ctor
  TextFragment(TextFragment&& b) noexcept;

  // move assignment operator
  TextFragment& operator=(TextFragment&& b) noexcept;

  // use these ctors instead of operator+.
  TextFragment(const TextFragment& a, const TextFragment& b) noexcept;
  TextFragment(const TextFragment& a, const TextFragment& b, const TextFragment& c) noexcept;
  TextFragment(const TextFragment& a, const TextFragment& b, const TextFragment& c,
               const TextFragment& d) noexcept;
  TextFragment(const TextFragment& a, const TextFragment& b, const TextFragment& c,
               const TextFragment& d, const TextFragment& e) noexcept;
  TextFragment(const TextFragment& a, const TextFragment& b, const TextFragment& c,
               const TextFragment& d, const TextFragment& e, const TextFragment& f) noexcept;
  TextFragment(const TextFragment& a, const TextFragment& b, const TextFragment& c,
               const TextFragment& d, const TextFragment& e, const TextFragment& f,
               const TextFragment& g) noexcept;
  TextFragment(const TextFragment& a, const TextFragment& b, const TextFragment& c,
               const TextFragment& d, const TextFragment& e, const TextFragment& f,
               const TextFragment& g, const TextFragment& h) noexcept;

  ~TextFragment() noexcept;

  explicit operator bool() const { return size_ > 0; }

  size_t lengthInBytes() const;

  size_t lengthInCodePoints() const;

  Iterator begin() const;
  Iterator end() const;

  inline const char* getText() const { return pText_; }

  inline bool beginsWith(const TextFragment& fb) const
  {
    size_t lenA = lengthInBytes();
    size_t lenB = fb.lengthInBytes();
    if (lenB > lenA) return false;
    for (int i = 0; i < lenB; ++i)
    {
      if (pText_[i] != fb.pText_[i])
      {
        return false;
      }
    }
    return true;
  }

  inline bool endsWith(const TextFragment& fb) const
  {
    size_t lenA = lengthInBytes();
    size_t lenB = fb.lengthInBytes();
    if (lenB > lenA) return false;
    for (int i = 0; i < lenB; ++i)
    {
      if (pText_[lenA - lenB + i] != fb.pText_[i])
      {
        return false;
      }
    }
    return true;
  }

 private:
  void _allocate(size_t size) noexcept;
  void _construct(const char* s1, size_t len1, const char* s2 = nullptr, size_t len2 = 0,
                  const char* s3 = nullptr, size_t len3 = 0, const char* s4 = nullptr,
                  size_t len4 = 0) noexcept;
  void _construct2(const char* s1, size_t len1, const char* s2 = nullptr, size_t len2 = 0,
                   const char* s3 = nullptr, size_t len3 = 0, const char* s4 = nullptr,
                   size_t len4 = 0, const char* s5 = nullptr, size_t len5 = 0,
                   const char* s6 = nullptr, size_t len6 = 0, const char* s7 = nullptr,
                   size_t len7 = 0, const char* s8 = nullptr, size_t len8 = 0) noexcept;
  void _nullTerminate() noexcept;
  void _dispose() noexcept;
  void _moveDataFromOther(TextFragment& b);

  // TODO these things could share space, as in SmallStackBuffer
  char localText_[kShortFragmentSizeInChars];
  char* pText_{localText_};

  // size of data in bytes, without null terminator
  size_t size_{0};
};

// ----------------------------------------------------------------
// Text - a placeholder for more features later like localization

typedef TextFragment Text;

// ----------------------------------------------------------------
// helper functions

inline bool compareSizedCharArrays(const char* pA, size_t lenA, const char* pB, size_t lenB)
{
  if (lenA != lenB) return false;
  if ((lenA == 0) && (lenB == 0)) return true;

  for (size_t n = 0; n < lenA; ++n)
  {
    if (pA[n] != pB[n])
    {
      return false;
    }
  }

  return true;
}

inline bool operator==(const TextFragment a, const TextFragment b)
{
  return compareSizedCharArrays(a.getText(), a.lengthInBytes(), b.getText(), b.lengthInBytes());
}

inline bool operator!=(TextFragment a, TextFragment b) { return !(a == b); }

inline std::ostream& operator<<(std::ostream& out, const TextFragment& r)
{
  const char* c = r.getText();
  out << c;
  return out;
}

bool validateCodePoint(CodePoint c);

std::vector<uint8_t> textToByteVector(TextFragment frag);
TextFragment byteVectorToText(const std::vector<uint8_t>& v);

std::vector<CodePoint> textToCodePoints(TextFragment frag);
TextFragment codePointsToText(std::vector<CodePoint> cv);

inline bool operator==(const TextFragment& a, const char* b) { return a == TextFragment(b); }
inline bool operator==(const char* a, const TextFragment& b) { return TextFragment(a) == b; }
inline bool operator!=(const TextFragment& a, const char* b) { return !(a == b); }
inline bool operator!=(const char* a, const TextFragment& b) { return !(a == b); }

inline uint64_t hash(const TextFragment& a)
{
  const char* c = a.getText();
  return fnv1aSubstring(c, strlen(c));
}

}  // namespace ml
