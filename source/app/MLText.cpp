// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLText.h"

#include <cstring>
#include <iostream>
#include <vector>

#include "MLMemoryUtils.h"
#include "utf.hpp"

namespace ml
{
// Iterator::Impl

class TextFragment::Iterator::Impl
{
  friend class TextFragment::Iterator;
  friend bool operator!=(Iterator lhs, Iterator rhs);
  friend bool operator==(Iterator lhs, Iterator rhs);

  utf::codepoint_iterator<const char*> _utf8Iter;

 public:
  Impl(const char* pos) : _utf8Iter(utf::codepoint_iterator<const char*>(pos)) {}
  Impl(const utf::codepoint_iterator<const char*>& utf_iter) : _utf8Iter(utf_iter) {}
};

// Iterator

TextFragment::Iterator::Iterator(const char* pos) { pImpl = std::unique_ptr<Impl>(new Impl(pos)); }

TextFragment::Iterator::Iterator(const Iterator& it)  // = default;
{
  pImpl = std::unique_ptr<Impl>(new Impl(it.pImpl->_utf8Iter));
}

TextFragment::Iterator::~Iterator() = default;

CodePoint TextFragment::Iterator::operator*() { return pImpl->_utf8Iter.operator*(); }

//		CodePoint operator->() { return _utf8Iter.operator->(); }

TextFragment::Iterator& TextFragment::Iterator::operator++()
{
  pImpl->_utf8Iter.operator++();
  return *this;
}

CodePoint TextFragment::Iterator::operator++(int i)
{
  CodePoint preIncrementValue = pImpl->_utf8Iter.operator*();
  pImpl->_utf8Iter.operator++(i);
  return preIncrementValue;
}

bool operator!=(TextFragment::Iterator lhs, TextFragment::Iterator rhs)
{
  return lhs.pImpl->_utf8Iter != rhs.pImpl->_utf8Iter;
}

bool operator==(TextFragment::Iterator lhs, TextFragment::Iterator rhs)
{
  return !(lhs.pImpl->_utf8Iter != rhs.pImpl->_utf8Iter);
}

// TextFragment

TextFragment::TextFragment() noexcept
{
  mSize = 0;
  mpText = mLocalText;
  nullTerminate();
}

TextFragment::TextFragment(const char* pChars) noexcept
{
  create(strlen(pChars));
  // a bad alloc will result in this being a null object.
  // copy the input string into local storage
  if (mpText)
  {
    std::copy(pChars, pChars + mSize, mpText);
    nullTerminate();
  }
}

// this ctor can be used to save the work of counting the length if we have a
// length already, as with static HashedCharArrays.
TextFragment::TextFragment(const char* pChars, size_t len) noexcept
{
  create(len);
  if (mpText)
  {
    std::copy(pChars, pChars + mSize, mpText);
    nullTerminate();
  }
}

TextFragment::TextFragment(CodePoint c) noexcept
{
  if (!validateCodePoint(c))
  {
    c = 0x2639;  // sad face
  }
  // all possible codepoints fit into local text
  char* end = utf::internal::utf_traits<utf::utf8>::encode(c, mLocalText);
  mSize = end - mLocalText;
  mpText = mLocalText;
  nullTerminate();
}

size_t TextFragment::lengthInBytes() const { return mSize; }

size_t TextFragment::lengthInCodePoints() const
{
  utf::stringview<const char*> sv(mpText, mpText + mSize);
  return sv.codepoints();
}

TextFragment::Iterator TextFragment::begin() const { return TextFragment::Iterator(getText()); }

TextFragment::Iterator TextFragment::end() const { return Iterator(getText() + lengthInBytes()); }

TextFragment::TextFragment(const TextFragment& a) noexcept
{
  construct(a.getText(), a.lengthInBytes());
}

// just copy the data. If we want to optimize and use reference-counted strings
// at some point, look at fix_str for ideas.
TextFragment& TextFragment::operator=(const TextFragment& b) noexcept
{
  if (this != &b)
  {
    dispose();
    create(b.mSize);
    if (mpText)
    {
      const char* bText = b.mpText;
      std::copy(bText, bText + mSize, mpText);
      nullTerminate();
    }
  }
  return *this;
}

TextFragment::TextFragment(TextFragment&& b) noexcept { moveDataFromOther(b); }

TextFragment& TextFragment::operator=(TextFragment&& b) noexcept
{
  dispose();
  moveDataFromOther(b);
  return *this;
}

// multiple-fragment constructors, used instead of operator+
TextFragment::TextFragment(const TextFragment& a, const TextFragment& b) noexcept
{
  construct(a.getText(), a.lengthInBytes(), b.getText(), b.lengthInBytes());
}

TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2,
                           const TextFragment& t3) noexcept
{
  construct(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(),
            t3.lengthInBytes());
}

TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2, const TextFragment& t3,
                           const TextFragment& t4) noexcept
{
  construct(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(),
            t3.lengthInBytes(), t4.getText(), t4.lengthInBytes());
}

TextFragment::~TextFragment() noexcept { dispose(); }

void TextFragment::construct(const char* s1, size_t len1, const char* s2, size_t len2,
                             const char* s3, size_t len3, const char* s4, size_t len4) noexcept
{
  create(len1 + len2 + len3 + len4);
  if (mpText)
  {
    if (len1) std::copy(s1, s1 + len1, mpText);
    if (len2) std::copy(s2, s2 + len2, mpText + len1);
    if (len3) std::copy(s3, s3 + len3, mpText + len1 + len2);
    if (len4) std::copy(s4, s4 + len4, mpText + len1 + len2 + len3);
    nullTerminate();
  }
}

void TextFragment::create(size_t size) noexcept
{
  mSize = size;
  const size_t nullTerminatedSize = size + 1;
  if (nullTerminatedSize > kShortFragmentSizeInChars)
  {
    mpText = static_cast<char*>(malloc(nullTerminatedSize));
  }
  else
  {
    mpText = mLocalText;
  }
}

void TextFragment::nullTerminate() noexcept { mpText[mSize] = 0; }

void TextFragment::dispose() noexcept
{
  if (mpText)
  {
    assert(mpText[mSize] == 0);
    if (mpText != mLocalText)
    {
      // free an external text. If the alloc has failed the ptr might be 0,
      // which is OK
      free(mpText);
    }
    mpText = 0;
  }
}

void TextFragment::moveDataFromOther(TextFragment& b)
{
  mSize = b.mSize;
  if (mSize >= kShortFragmentSizeInChars)
  {
    // move the data
    mpText = b.mpText;
  }
  else
  {
    /*
     TODO use SmallStackBuffer! and test
     */

    // point to local storage and copy data
    mpText = mLocalText;
    std::copy(b.mLocalText, b.mLocalText + mSize, mLocalText);
    nullTerminate();
  }

  // mark b as empty, nothing to dispose
  b.mpText = b.mLocalText;
  b.mSize = 0;
  b.nullTerminate();
}

bool validateCodePoint(CodePoint c) { return utf::internal::validate_codepoint(c); }

// return UTF-8 encoded vector of bytes without null terminator
std::vector<uint8_t> textToByteVector(TextFragment frag)
{
  return std::vector<uint8_t>(frag.getText(), frag.getText() + frag.lengthInBytes());
}

TextFragment byteVectorToText(const std::vector<uint8_t>& v)
{
  if (!v.size()) return TextFragment();
  const uint8_t* p = v.data();
  return TextFragment(reinterpret_cast<const char*>(p), v.size());
}

// TODO small stack objects here to make random access class, don't use
// std::vector
std::vector<CodePoint> textToCodePoints(TextFragment frag)
{
  std::vector<CodePoint> r;
  for (CodePoint c : frag)
  {
    r.push_back(c);
  }
  return r;
}

TextFragment codePointsToText(std::vector<CodePoint> cv)
{
  auto sv = utf::make_stringview(cv.begin(), cv.end());
  std::vector<char> outVec;
  sv.to<utf::utf8>(std::back_inserter(outVec));
  return TextFragment(outVec.data(), outVec.size());
}

}  // namespace ml
