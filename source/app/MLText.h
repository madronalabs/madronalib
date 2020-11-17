//
//  MLText.h
//  madronalib
//
//  Created by Randy Jones on 5/21/16.
//

// assignable but otherwise immutable UTF-8 text object class

#pragma once

#include <memory>
#include <string>  // to remove
#include <vector>

namespace ml
{
// ----------------------------------------------------------------
// TextFragment - a minimal, immutable string class. Guaranteed not to allocate
// heap if the length in bytes is below kShortFragmentSize.

static constexpr int kShortFragmentSizeInCodePoints = 16;
static constexpr int kShortFragmentSizeInChars = kShortFragmentSizeInCodePoints * 4;

using CodePoint = char32_t;

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
    //	Iterator(Iterator&&) = default;
    Iterator(const Iterator&);
    //	Iterator& operator=(Iterator&&); // defined in the implementation file
    Iterator& operator=(const Iterator&) = delete;

    CodePoint operator*();
    //		CodePoint operator->() { return _utf8Iter.operator->(); }
    Iterator& operator++();
    CodePoint operator++(int i);

    friend bool operator!=(Iterator lhs, Iterator rhs);
    friend bool operator==(Iterator lhs, Iterator rhs);
  };

  TextFragment() noexcept;

  /*
   this could be a good idea but the (const char*) ctor is taking precedence,
  revisit template<size_t N> TextFragment(const char(&p)[N]) noexcept : mSize(N)
  {
          std::cout << "?";
          create();
          if(mpText)
          {
                  std::copy(p, p + mSize, mpText);
                  nullTerminate();
          }
  }
   */

  //
  TextFragment(const char* pChars) noexcept;

  // this ctor can be used to save the work of counting the length of the input
  // if we know it already, as with static HashedCharArrays.
  TextFragment(const char* pChars, size_t len) noexcept;

  // single code point ctor
  TextFragment(CodePoint c) noexcept;

  // copy ctor
  TextFragment(const TextFragment& a) noexcept;

  // copy assignment operator: TextFragment is assignable but otherwise
  // immutable.
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

  ~TextFragment() noexcept;

  explicit operator bool() const { return mSize > 0; }

  size_t lengthInBytes() const;

  size_t lengthInCodePoints() const;

  Iterator begin() const;
  Iterator end() const;

  inline const char* getText() const { return mpText; }

  inline bool beginsWith(const TextFragment& fb) const
  {
    size_t lenA = lengthInBytes();
    size_t lenB = fb.lengthInBytes();
    if (lenB > lenA) return false;
    for (int i = 0; i < lenB; ++i)
    {
      if (mpText[i] != fb.mpText[i])
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
      if (mpText[lenA - lenB + i] != fb.mpText[i])
      {
        return false;
      }
    }
    return true;
  }

  // deprecated! MLTEST
  inline std::string toString() const { return std::string(mpText); }

 private:
  void construct(const char* s1, size_t len1, const char* s2 = nullptr, size_t len2 = 0,
                 const char* s3 = nullptr, size_t len3 = 0, const char* s4 = nullptr,
                 size_t len4 = 0) noexcept;

  void create(size_t size) noexcept;
  void nullTerminate() noexcept;
  void dispose() noexcept;
  void moveDataFromOther(TextFragment& b);

  // TODO these things could share space, as in SmallStackBuffer
  char* mpText;
  char mLocalText[kShortFragmentSizeInChars];

  // size of data in bytes, without null terminator
  size_t mSize;
};

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

// TODO made operator== a free function-	do likewise for other classes

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

// ----------------------------------------------------------------
// Text - a placeholder for more features later like localization

typedef TextFragment Text;

// ----------------------------------------------------------------
// functions

bool validateCodePoint(CodePoint c);

std::vector<uint8_t> textToByteVector(TextFragment frag);
TextFragment byteVectorToText(const std::vector<uint8_t>& v);

std::vector<CodePoint> textToCodePoints(TextFragment frag);
TextFragment codePointsToText(std::vector<CodePoint> cv);

}  // namespace ml
