//
//  MLTextUtils.h
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "MLSymbol.h"

namespace ml
{
namespace textUtils
{
bool isDigit(CodePoint c);
bool isASCII(CodePoint c);
bool isLatin(CodePoint c);
bool isWhitespace(CodePoint c);
bool isCJK(CodePoint c);

char* spaceStr(size_t numIndents);

// ----------------------------------------------------------------
// TextFragment utilities

TextFragment naturalNumberToText(int i);
int textToNaturalNumber(const TextFragment& frag);

TextFragment floatNumberToText(float f, int precision = 5);
float textToFloatNumber(const TextFragment& frag);

int findFirst(const TextFragment& frag, const CodePoint c);
int findLast(const TextFragment& frag, const CodePoint c);

int findFirst(const TextFragment& frag, std::function<bool(CodePoint)> f);
int findLast(const TextFragment& frag, std::function<bool(CodePoint)> f);

// join a vector of fragments into one fragment.
TextFragment join(const std::vector<TextFragment>& vec);

// join a vector of fragments into one fragment, with delimiter added in
// between.
TextFragment join(const std::vector<TextFragment>& vec, CodePoint delimiter);

// Return a new TextFragment consisting of the codepoints from indices start to
// (end - 1) in the input frag.
TextFragment subText(const TextFragment& frag, size_t start, size_t end);

// given a fragment and a mapping function on code points, return a new fragment
// with the function applied to each code point.
TextFragment map(const TextFragment& frag, std::function<CodePoint(CodePoint)> f);

// given a fragment and a reducing function on code points, return a new
// fragment with only the code points for which the function returns true.
TextFragment reduce(const TextFragment& frag, std::function<bool(CodePoint)> f);

// given a fragment and a delimiter, return a std::vector of fragments
// containing the texts between instances of the delimiter. If no delimiters are
// found the original fragment is returned.
std::vector<TextFragment> split(TextFragment frag, CodePoint delimiter = '\n');

// Return the prefix of the input frag as a new TextFragment, stripping the last
// dot and any codepoints after it.
TextFragment stripFileExtension(const TextFragment& frag);

// If the input fragment contains a slash, return a new TextFragment containing
// any characters after the final slash. Else return the input.
TextFragment getShortFileName(const TextFragment& frag);

// Return a new TextFragment containing any characters up to a final slash.
TextFragment getPath(const TextFragment& frag);

Symbol bestScriptForTextFragment(const TextFragment& frag);

TextFragment stripWhitespaceAtEnds(const TextFragment& frag);
TextFragment stripAllWhitespace(const TextFragment& frag);

TextFragment base64Encode(const std::vector<uint8_t>& b);
std::vector<uint8_t> base64Decode(const TextFragment& b);

std::vector<uint8_t> AES256CBCEncode(const std::vector<uint8_t>& plaintext,
                                     const std::vector<uint8_t>& key,
                                     const std::vector<uint8_t>& iv);
std::vector<uint8_t> AES256CBCDecode(const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& key,
                                     const std::vector<uint8_t>& iv);

// perform case-insensitive compare of fragments and return (a < b).
// TODO collate other languages better using miniutf library.
bool collate(const TextFragment& a, const TextFragment& b);

// ----------------------------------------------------------------
// Symbol utilities

Symbol addFinalNumber(Symbol sym, int n);
Symbol stripFinalNumber(Symbol sym);
int getFinalNumber(Symbol sym);
Symbol stripFinalCharacter(Symbol sym);

std::vector<Symbol> vectorOfNonsenseSymbols(int len);

struct SymbolCollator
{
  bool operator()(const Symbol& a, const Symbol& b) const
  {
    return collate(a.getTextFragment(), b.getTextFragment());
  }
};

// ----------------------------------------------------------------
// NameMaker
// a utility to make many short, unique, human-readable names when they are
// needed.

class NameMaker
{
  static const int maxLen = 64;

 public:
  NameMaker() : index(0){};
  ~NameMaker(){};

  // return the next name as a symbol, having added it to the symbol table.
  const TextFragment nextName();

 private:
  int index;
  char buf[maxLen];
};

// ----------------------------------------------------------------
// std library helpers

template <typename T>
T getElementChecked(const std::vector<T> vec, int index) noexcept
{
  return (vec.size() > index ? vec[index] : T());
}

// ----------------------------------------------------------------
// number formatter

ml::Text formatNumber(const float number, const int digits, const int precision, const bool doSign,
                      Symbol mode = "default") throw();

}  // namespace textUtils
}  // namespace ml
