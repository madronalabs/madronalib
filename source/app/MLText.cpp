// madronalib: a C++ framework for DSP applications.
// Copyright (c) 2020-2022 Madrona Labs LLC. http://www.madronalabs.com
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

  utf::codepoint_iterator<const char*> utf8Iter_;

 public:
  Impl(const char* pos) : utf8Iter_(utf::codepoint_iterator<const char*>(pos)) {}
  Impl(const utf::codepoint_iterator<const char*>& utf_iter) : utf8Iter_(utf_iter) {}
};

// Iterator

TextFragment::Iterator::Iterator(const char* pos) { pImpl = std::unique_ptr<Impl>(new Impl(pos)); }

TextFragment::Iterator::Iterator(const Iterator& it)  // = default;
{
  pImpl = std::unique_ptr<Impl>(new Impl(it.pImpl->utf8Iter_));
}

TextFragment::Iterator::~Iterator() = default;

CodePoint TextFragment::Iterator::operator*() { return pImpl->utf8Iter_.operator*(); }

TextFragment::Iterator& TextFragment::Iterator::operator++()
{
  pImpl->utf8Iter_.operator++();
  return *this;
}

CodePoint TextFragment::Iterator::operator++(int i)
{
  CodePoint preIncrementValue = pImpl->utf8Iter_.operator*();
  pImpl->utf8Iter_.operator++(i);
  return preIncrementValue;
}

bool operator!=(TextFragment::Iterator lhs, TextFragment::Iterator rhs)
{
  return lhs.pImpl->utf8Iter_ != rhs.pImpl->utf8Iter_;
}

bool operator==(TextFragment::Iterator lhs, TextFragment::Iterator rhs)
{
  return !(lhs.pImpl->utf8Iter_ != rhs.pImpl->utf8Iter_);
}

// TextFragment

TextFragment::TextFragment() noexcept
{
  size_ = 0;
  pText_ = localText_;
  _nullTerminate();
}

TextFragment::TextFragment(const char* pChars) noexcept
{
  if (pChars)
  {
    _allocate(strlen(pChars));
    // a bad alloc will result in this being a null object.
    // copy the input string into local storage
    if (pText_)
    {
      std::copy(pChars, pChars + size_, pText_);
      _nullTerminate();
    }
  }
  else
  {
    // null input ptr will produce a null fragment.
    _nullTerminate();
  }
}

// this ctor can be used to save the work of counting the length if we have a
// length already, as with static HashedCharArrays.
TextFragment::TextFragment(const char* pChars, size_t len) noexcept
{
  _allocate(len);
  if (pText_)
  {
    std::copy(pChars, pChars + size_, pText_);
    _nullTerminate();
  }
}

TextFragment::TextFragment(CodePoint c) noexcept
{
  if (!validateCodePoint(c))
  {
    c = 0x2639;  // sad face
  }
  // all possible codepoints fit into local text
  char* end = utf::internal::utf_traits<utf::utf8>::encode(c, localText_);
  size_ = end - localText_;
  pText_ = localText_;
  _nullTerminate();
}

size_t TextFragment::lengthInBytes() const { return size_; }

size_t TextFragment::lengthInCodePoints() const
{
  utf::stringview<const char*> sv(pText_, pText_ + size_);
  return sv.codepoints();
}

TextFragment::Iterator TextFragment::begin() const { return TextFragment::Iterator(getText()); }

TextFragment::Iterator TextFragment::end() const { return Iterator(getText() + lengthInBytes()); }

TextFragment::TextFragment(const TextFragment& a) noexcept
{
  _construct(a.getText(), a.lengthInBytes());
}

// just copy the data. If we want to optimize and use reference-counted strings
// at some point, look at fix_str for ideas.
TextFragment& TextFragment::operator=(const TextFragment& b) noexcept
{
  if (this != &b)
  {
    _dispose();
    _allocate(b.size_);
    if (pText_)
    {
      const char* bText = b.pText_;
      std::copy(bText, bText + size_, pText_);
      _nullTerminate();
    }
  }
  return *this;
}

TextFragment::TextFragment(TextFragment&& b) noexcept { _moveDataFromOther(b); }

TextFragment& TextFragment::operator=(TextFragment&& b) noexcept
{
  _dispose();
  _moveDataFromOther(b);
  return *this;
}

// multiple-fragment constructors, used instead of operator+
TextFragment::TextFragment(const TextFragment& a, const TextFragment& b) noexcept
{
  _construct(a.getText(), a.lengthInBytes(), b.getText(), b.lengthInBytes());
}

TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2,
                           const TextFragment& t3) noexcept
{
  _construct(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(),
             t3.lengthInBytes());
}

TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2, const TextFragment& t3,
                           const TextFragment& t4) noexcept
{
  _construct(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(),
             t3.lengthInBytes(), t4.getText(), t4.lengthInBytes());
}

TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2, const TextFragment& t3,
                           const TextFragment& t4, const TextFragment& t5) noexcept
{
  _construct2(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(),
              t3.lengthInBytes(), t4.getText(), t4.lengthInBytes(), t5.getText(),
              t5.lengthInBytes());
}

TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2, const TextFragment& t3,
                           const TextFragment& t4, const TextFragment& t5,
                           const TextFragment& t6) noexcept
{
  _construct2(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(),
              t3.lengthInBytes(), t4.getText(), t4.lengthInBytes(), t5.getText(),
              t5.lengthInBytes(), t6.getText(), t6.lengthInBytes());
}

TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2, const TextFragment& t3,
                           const TextFragment& t4, const TextFragment& t5, const TextFragment& t6,
                           const TextFragment& t7) noexcept
{
  _construct2(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(),
              t3.lengthInBytes(), t4.getText(), t4.lengthInBytes(), t5.getText(),
              t5.lengthInBytes(), t6.getText(), t6.lengthInBytes(), t7.getText(),
              t7.lengthInBytes());
}

TextFragment::TextFragment(const TextFragment& t1, const TextFragment& t2, const TextFragment& t3,
                           const TextFragment& t4, const TextFragment& t5, const TextFragment& t6,
                           const TextFragment& t7, const TextFragment& t8) noexcept
{
  _construct2(t1.getText(), t1.lengthInBytes(), t2.getText(), t2.lengthInBytes(), t3.getText(),
              t3.lengthInBytes(), t4.getText(), t4.lengthInBytes(), t5.getText(),
              t5.lengthInBytes(), t6.getText(), t6.lengthInBytes(), t7.getText(),
              t7.lengthInBytes(), t8.getText(), t8.lengthInBytes());
}

TextFragment::~TextFragment() noexcept { _dispose(); }

void TextFragment::_construct(const char* s1, size_t len1, const char* s2, size_t len2,
                              const char* s3, size_t len3, const char* s4, size_t len4) noexcept
{
  _allocate(len1 + len2 + len3 + len4);
  if (pText_)
  {
    if (len1) std::copy(s1, s1 + len1, pText_);
    if (len2) std::copy(s2, s2 + len2, pText_ + len1);
    if (len3) std::copy(s3, s3 + len3, pText_ + len1 + len2);
    if (len4) std::copy(s4, s4 + len4, pText_ + len1 + len2 + len3);
    _nullTerminate();
  }
}

void TextFragment::_construct2(const char* s1, size_t len1, const char* s2, size_t len2,
                               const char* s3, size_t len3, const char* s4, size_t len4,
                               const char* s5, size_t len5, const char* s6, size_t len6,
                               const char* s7, size_t len7, const char* s8, size_t len8

                               ) noexcept
{
  _allocate(len1 + len2 + len3 + len4 + len5 + len6 + len7 + len8);
  if (pText_)
  {
    if (len1) std::copy(s1, s1 + len1, pText_);
    if (len2) std::copy(s2, s2 + len2, pText_ + len1);
    if (len3) std::copy(s3, s3 + len3, pText_ + len1 + len2);
    if (len4) std::copy(s4, s4 + len4, pText_ + len1 + len2 + len3);
    if (len5) std::copy(s5, s5 + len5, pText_ + len1 + len2 + len3 + len4);
    if (len6) std::copy(s6, s6 + len6, pText_ + len1 + len2 + len3 + len4 + len5);
    if (len7) std::copy(s7, s7 + len7, pText_ + len1 + len2 + len3 + len4 + len5 + len6);
    if (len8) std::copy(s8, s8 + len8, pText_ + len1 + len2 + len3 + len4 + len5 + len6 + len7);
    _nullTerminate();
  }
}

void TextFragment::_allocate(size_t size) noexcept
{
  size_ = size;
  const size_t nullTerminatedSize = size + 1;
  if (nullTerminatedSize > kShortFragmentSizeInChars)
  {
    pText_ = static_cast<char*>(malloc(nullTerminatedSize));
  }
  else
  {
    pText_ = localText_;
  }
}

void TextFragment::_nullTerminate() noexcept { pText_[size_] = 0; }

void TextFragment::_dispose() noexcept
{
  if (pText_)
  {
    assert(pText_[size_] == 0);
    if (pText_ != localText_)
    {
      // free an external text. If the alloc has failed the ptr might be 0,
      // which is OK
      free(pText_);
    }
    pText_ = 0;
  }
}

void TextFragment::_moveDataFromOther(TextFragment& b)
{
  size_ = b.size_;
  if (size_ >= kShortFragmentSizeInChars)
  {
    // move the data
    pText_ = b.pText_;
  }
  else
  {
    /*
     TODO use SmallStackBuffer! and test
     */

    // point to local storage and copy data
    pText_ = localText_;
    std::copy(b.localText_, b.localText_ + size_, localText_);
    _nullTerminate();
  }

  // mark b as empty, nothing to dispose
  b.pText_ = b.localText_;
  b.size_ = 0;
  b._nullTerminate();
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
