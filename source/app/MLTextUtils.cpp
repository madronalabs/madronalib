//
//  MLTextUtils.cpp
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#include "MLTextUtils.h"

#include <cstring>

#include "MLDSPScalarMath.h"
#include "MLMemoryUtils.h"
#include "aes256.h"
#include "utf.hpp"

namespace ml
{
namespace textUtils
{
static const int npos = -1;

bool isDigit(CodePoint c)
{
  if (c >= '0' && c <= '9') return true;
  return false;
}
bool isASCII(CodePoint c) { return (c < 0x7f); }

bool isLatin(CodePoint c)
{
  // includes Latin-1 Supplement
  return (c <= 0xFF);
}

bool isWhitespace(CodePoint ch)
{
  return (ch >= 0x0009 && ch <= 0x000D) || ch == 0x0020 || ch == 0x0085 || ch == 0x00A0 ||
         ch == 0x1680 || (ch >= 0x2000 && ch <= 0x200A) || ch == 0x2028 || ch == 0x2029 ||
         ch == 0x202F || ch == 0x205F || ch == 0x3000;
}

bool isCJK(CodePoint ch)
{
  return (ch >= 0x4E00 && ch <= 0x9FBF)      // CJK Unified Ideographs
         || (ch >= 0x2E80 && ch <= 0x2FDF)   // CJK Radicals Supplement & Kangxi Radicals
         || (ch >= 0x2FF0 && ch <= 0x30FF)   // Ideographic Description Characters, CJK Symbols
                                             // and Punctuation & Japanese
         || (ch >= 0x3100 && ch <= 0x31BF)   // Korean
         || (ch >= 0xAC00 && ch <= 0xD7AF)   // Hangul Syllables
         || (ch >= 0xF900 && ch <= 0xFAFF)   // CJK Compatibility Ideographs
         || (ch >= 0xFE30 && ch <= 0xFE4F)   // CJK Compatibility Forms
         || (ch >= 0x31C0 && ch <= 0x4DFF);  // Other exiensions
}

int digitsToNaturalNumber(const char32_t* p)
{
  constexpr int kMaxDigits = 16;

  if (!p) return 0;
  int v = 0;
  int l = 0;
  int d;
  char c;

  while (p[l])
  {
    c = p[l];
    if (c >= '0' && c <= '9')
      d = (c - '0');
    else
      break;
    v = (v * 10) + d;
    l++;
    if (l >= kMaxDigits) return -1;
  }
  return v;
}

int textToNaturalNumber(const TextFragment& frag)
{
  std::vector<CodePoint> vec = textToCodePoints(frag);
  return digitsToNaturalNumber(vec.data());
}

TextFragment naturalNumberToText(int i)
{
  constexpr int kMaxDigits = 16;

  char buf[kMaxDigits]{};
  char* p = buf + kMaxDigits - 1;
  char* end = p;

  // null-terminate the string
  *end = 0;

  // work backwards
  do
  {
    p--;
    if (p < buf) return "overflow";
    *p = '0' + (i % 10);
    i /= 10;
  } while (i != 0);
  return (TextFragment(p, end - p));
}

// numeric

TextFragment floatNumberToText(float f, int precision)
{
  // const float maxFloat = std::numeric_limits<float>::max();
  constexpr int kMaxPrecision = 10;
  constexpr int kScientificStart = 5;
  constexpr int kMaxDigits = 32;
  constexpr int kTableZeroOffset = 38;
  constexpr float powersOfTen[kTableZeroOffset * 2 + 1]{
      1e-38, 1e-37, 1e-36, 1e-35, 1e-34, 1e-33, 1e-32, 1e-31, 1e-30, 1e-29, 1e-28, 1e-27, 1e-26,
      1e-25, 1e-24, 1e-23, 1e-22, 1e-21, 1e-20, 1e-19, 1e-18, 1e-17, 1e-16, 1e-15, 1e-14, 1e-13,
      1e-12, 1e-11, 1e-10, 1e-09, 1e-08, 1e-07, 1e-06, 1e-05, 1e-04, 1e-03, 1e-02, 1e-01, 1e+00,
      1e+01, 1e+02, 1e+03, 1e+04, 1e+05, 1e+06, 1e+07, 1e+08, 1e+09, 1e+10, 1e+11, 1e+12, 1e+13,
      1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20, 1e+21, 1e+22, 1e+23, 1e+24, 1e+25, 1e+26,
      1e+27, 1e+28, 1e+29, 1e+30, 1e+31, 1e+32, 1e+33, 1e+34, 1e+35, 1e+36, 1e+37, 1e+38};

  char buf[kMaxDigits];
  char* writePtr = buf;
  float value = f;
  const int p = std::min(precision, kMaxPrecision);
  const float epsilon =
      std::max(fabs(f * powersOfTen[kTableZeroOffset - p]), std::numeric_limits<float>::min());

  if (isnan(f))
  {
    *writePtr++ = 'n';
    *writePtr++ = 'a';
    *writePtr++ = 'n';
  }
  else
  {
    if (value < 0)
    {
      value = -value;
      *writePtr++ = '-';
    }

    if (value > powersOfTen[kTableZeroOffset * 2])
    {
      *writePtr++ = 'i';
      *writePtr++ = 'n';
      *writePtr++ = 'f';
    }
    else if (value < powersOfTen[0])
    {
      *writePtr++ = '0';
      *writePtr++ = '.';
    }
    else
    {
      // get the exponent using linear search, starting from center
      int y = kTableZeroOffset;
      while (value > powersOfTen[y])
      {
        y++;
      }
      while (value < powersOfTen[y])
      {
        y--;
      }
      int exponent = y - kTableZeroOffset;
      int absExponent = std::abs(exponent);

      if (absExponent < kScientificStart)
      // write in decimal notation
      {
        // first write any leading zeroes
        if (exponent < -1)
        {
          *writePtr++ = '0';
          *writePtr++ = '.';
          int zeroes = -exponent - 1;
          for (int i = 0; i < zeroes; ++i)
          {
            *writePtr++ = '0';
          }
        }
        else if (exponent == -1)
        {
          *writePtr++ = '0';
        }

        // then write nonzero digits
        do
        {
          if (exponent == -1)
          {
            *writePtr++ = '.';
          }
          int onesInt = truncf(value * powersOfTen[kTableZeroOffset - exponent]);
          *writePtr++ = '0' + onesInt;
          value = value - onesInt * powersOfTen[kTableZeroOffset + exponent];
          exponent--;
        } while ((value > epsilon) || (exponent >= 0));
      }
      else
      // write in scientific notation
      {
        const char exponentSign = exponent >= 0 ? '+' : '-';

        // write mantissa
        int onesInt = value * powersOfTen[kTableZeroOffset - exponent];
        *writePtr++ = '0' + onesInt;
        *writePtr++ = '.';
        while (value > epsilon)
        {
          value = value - onesInt * powersOfTen[kTableZeroOffset + exponent];
          exponent--;
          onesInt = value * powersOfTen[kTableZeroOffset - exponent];
          *writePtr++ = '0' + onesInt;
        }

        // write exponent
        *writePtr++ = 'e';
        *writePtr++ = exponentSign;
        *writePtr++ = '0' + absExponent / 10;
        *writePtr++ = '0' + absExponent % 10;
      }
    }
  }
  return TextFragment(buf, writePtr - buf);
}

bool fragmentContainsCodePoint(TextFragment f, CodePoint cp)
{
  for (const CodePoint c : f)
  {
    if (c == cp) return true;
  }
  return false;
}

float textToFloatNumber(const TextFragment& frag)
{
  float sign = 1;
  float wholePart = 0, fracPart = 0, fracPlace = 1;
  float exponentSign = 1, exponent = 0;
  bool hasExp = false;
  auto it = frag.begin();
  const TextFragment digits{"0123456789"};
  std::vector<std::pair<TextFragment, std::function<void()> > > segments{
      {"NaN", [&]() { wholePart = std::numeric_limits<float>::quiet_NaN(); }},
      {"-", [&]() { sign = -sign; }},
      {"inf", [&]() { wholePart = std::numeric_limits<float>::infinity(); }},
      {digits, [&]() { wholePart = wholePart * 10.0f + ((*it) - '0'); }},
      {".", [&]() {}},
      {digits, [&]() { fracPart += ((*it) - '0') * (fracPlace *= 0.1f); }},
      {"e+", [&]() { hasExp = true; }},
      {"-", [&]() { exponentSign = -exponentSign; }},
      {digits, [&]() { exponent = exponent * 10.0f + ((*it) - '0'); }}};

  for (auto segment : segments)
  {
    while (fragmentContainsCodePoint(segment.first, *it))
    {
      segment.second();
      ++it;
    }
  }

  float base = sign * (wholePart + fracPart);
  return hasExp ? base * powf(10.f, exponent * exponentSign) : base;
}

int findFirst(const TextFragment& frag, const CodePoint b)
{
  int r = npos;
  if (!frag) return r;
  int i = 0;
  for (const CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return r;
    if (c == b)
    {
      r = i;
      break;
    }
    i++;
  }
  return r;
}

int findLast(const TextFragment& frag, const CodePoint b)
{
  int r = npos;
  if (!frag) return r;
  int i = 0;
  for (const CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return r;
    if (c == b)
    {
      r = i;
    }
    i++;
  }
  return r;
}

int findFirst(const TextFragment& frag, std::function<bool(CodePoint)> matchFn)
{
  int r = npos;
  if (!frag) return r;
  int i = 0;
  for (const CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return r;
    if (matchFn(c))
    {
      r = i;
      break;
    }
    i++;
  }
  return r;
}

// TODO dumb, have to call matchFn on each code point because we have no reverse
// iterator
int findLast(const TextFragment& frag, std::function<bool(CodePoint)> matchFn)
{
  int r = npos;
  if (!frag) return r;
  int i = 0;
  for (const CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return r;
    if (matchFn(c))
    {
      r = i;
    }
    i++;
  }
  return r;
}

TextFragment subText(const TextFragment& frag, size_t start, size_t end)
{
  // this impl does an unneccesary copy, to keep TextFragment very simple for
  // now.
  if (!frag) return TextFragment();
  if (start >= end) return TextFragment();

  // temp buffer big enough to hold whole input fragment if needed.
  // we won't know the output fragment size in bytes until iterating the code
  // points.
  size_t len = frag.lengthInBytes();
  SmallStackBuffer<char, kShortFragmentSizeInChars> temp(len);
  char* buf = temp.data();
  char* pb = buf;

  auto first = TextFragment::Iterator(frag.getText());
  auto it = first;
  for (int i = 0; i < start; ++i)
  {
    ++it;
  }

  for (int i = 0; i < end - start; ++i)
  {
    // write the codepoint as UTF-8 to the buffer
    if (!validateCodePoint(*it)) return TextFragment();
    pb = utf::internal::utf_traits<utf::utf8>::encode(*it, pb);
    ++it;
  }

  return TextFragment(buf, pb - buf);
}

TextFragment map(const TextFragment& frag, std::function<CodePoint(CodePoint)> f)
{
  if (!frag) return TextFragment();
  std::vector<CodePoint> vec = textToCodePoints(frag);
  std::transform(vec.begin(), vec.end(), vec.begin(), f);
  return codePointsToText(vec);
}

TextFragment reduce(const TextFragment& frag, std::function<bool(CodePoint)> matchFn)
{
  if (!frag) return TextFragment();
  size_t len = frag.lengthInBytes();
  SmallStackBuffer<char, kShortFragmentSizeInChars> temp(len);
  char* buf = temp.data();
  char* pb = buf;

  for (const CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return TextFragment();
    if (matchFn(c))
    {
      pb = utf::internal::utf_traits<utf::utf8>::encode(c, pb);
    }
  }

  return TextFragment(buf, pb - buf);
}

std::vector<TextFragment> split(TextFragment frag, CodePoint delimiter)
{
  std::vector<TextFragment> output;
  int start = 0;
  int end = 0;
  int pieceLen = 0;
  for (const CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return std::vector<TextFragment>();
    pieceLen++;
    end++;
    if (c == delimiter)
    {
      if (pieceLen > 1)
      {
        output.push_back(subText(frag, start, end - 1));
      }
      start = end;
      pieceLen = 0;
    }
  }
  if (pieceLen > 0)
  {
    output.push_back(subText(frag, start, end));
  }
  return output;
}

TextFragment join(const std::vector<TextFragment>& vec)
{
  TextFragment sum;
  size_t len = vec.size();
  for (int i = 0; i < len; ++i)
  {
    TextFragment frag = vec[i];
    sum = TextFragment(sum, vec[i]);
  }
  return sum;
}

TextFragment join(const std::vector<TextFragment>& vec, CodePoint delimiter)
{
  TextFragment delimFrag(delimiter);
  TextFragment sum;
  size_t len = vec.size();
  for (int i = 0; i < len; ++i)
  {
    TextFragment frag = vec[i];
    sum = TextFragment(sum, vec[i]);
    if ((i >= 0) && (i < len - 1))
    {
      sum = TextFragment(sum, delimFrag);
    }
  }
  return sum;
}

TextFragment stripFileExtension(const TextFragment& frag)
{
  int dotLoc = findLast(frag, '.');
  if (dotLoc >= 0)
  {
    return subText(frag, 0, dotLoc);
  }
  return frag;
}

TextFragment getShortFileName(const TextFragment& frag)
{
  int slashLoc = findLast(frag, '/');
  if (slashLoc >= 0)
  {
    return subText(frag, slashLoc + 1, frag.lengthInCodePoints());
  }
  return frag;
}

TextFragment getPath(const TextFragment& frag)
{
  int slashLoc = findLast(frag, '/');
  if (slashLoc >= 0)
  {
    return subText(frag, 0, slashLoc);
  }
  return frag;
}

// TODO extend to recognize Cyrillic and other scripts
Symbol bestScriptForTextFragment(const TextFragment& frag)
{
  for (const CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return "unknown";
    // if there are any CJK characters, return CJK
    if (isCJK(c))
    {
      return "cjk";
    }
    else if (!isLatin(c))
    {
      return "unknown";
    }
  }
  return "latin";
}

static const char base64table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
int indexOf(const char* str, char c)
{
  int r = -1;
  size_t len = strlen(str);
  for (size_t i = 0; i < len; ++i)
  {
    if (str[i] == c)
    {
      r = (int)i;
      break;
    }
  }
  return r;
}

TextFragment base64Encode(const std::vector<uint8_t>& in)
{
  size_t len = in.size();
  std::vector<char> out;
  int b;
  for (size_t i = 0; i < len; i += 3)
  {
    b = (in[i] & 0xFC) >> 2;
    out.push_back(base64table[b]);
    b = (in[i] & 0x03) << 4;
    if (i + 1 < len)
    {
      b |= (in[i + 1] & 0xF0) >> 4;
      out.push_back(base64table[b]);
      b = (in[i + 1] & 0x0F) << 2;
      if (i + 2 < len)
      {
        b |= (in[i + 2] & 0xC0) >> 6;
        out.push_back(base64table[b]);
        b = in[i + 2] & 0x3F;
        out.push_back(base64table[b]);
      }
      else
      {
        out.push_back(base64table[b]);
        out.push_back('=');
      }
    }
    else
    {
      out.push_back(base64table[b]);
      out.push_back('=');
      out.push_back('=');
    }
  }
  out.push_back(0);
  return TextFragment(out.data());
}

std::vector<uint8_t> base64Decode(const TextFragment& frag)
{
  size_t len = frag.lengthInBytes();
  if (len % 4) return std::vector<uint8_t>();
  std::vector<uint8_t> decoded;
  const char* inChars = frag.getText();
  int b[4];
  for (int i = 0; i < len; i += 4)
  {
    for (int j = 0; j < 4; ++j)
    {
      b[j] = indexOf(base64table, inChars[i + j]);
    }
    decoded.push_back((b[0] << 2) | (b[1] >> 4));
    if (b[2] < 64)
    {
      decoded.push_back((b[1] << 4) | (b[2] >> 2));
      if (b[3] < 64)
      {
        decoded.push_back((b[2] << 6) | b[3]);
      }
    }
  }
  return decoded;
}

TextFragment stripWhitespaceAtEnds(const TextFragment& frag)
{
  std::function<bool(CodePoint)> f([](CodePoint c) { return !isWhitespace(c); });
  int first = findFirst(frag, f);
  int last = findLast(frag, f);
  if ((first == npos) || (last == npos)) return TextFragment();
  return (subText(frag, first, last + 1));
}

TextFragment stripAllWhitespace(const TextFragment& frag)
{
  std::function<bool(CodePoint)> f([](CodePoint c) { return !isWhitespace(c); });
  return reduce(frag, f);
}

std::vector<uint8_t> AES256CBCEncode(const std::vector<uint8_t>& input,
                                     const std::vector<uint8_t>& key,
                                     const std::vector<uint8_t>& iv)
{
  if (!(input.size() > 0) || !(key.size() == 32) || !(iv.size() == 32))
    return std::vector<uint8_t>();

  aes256_context ctx;
  aes256_init(&ctx, key.data());

  const int blockSize = 16;
  size_t inputSize = input.size();
  size_t blocks = inputSize / blockSize + 1;
  size_t paddedSize = blockSize * (blocks);

  // add PKCS padding
  std::vector<uint8_t> plaintext = input;
  plaintext.resize(paddedSize);
  size_t padBytes = paddedSize - inputSize;
  for (size_t i = inputSize; i < paddedSize; ++i)
  {
    plaintext[i] = padBytes;
  }

  std::vector<uint8_t> ciphertext(paddedSize);
  uint8_t currentIV[blockSize];
  uint8_t workVector[blockSize];

  for (size_t i = 0; i < blockSize; ++i)
  {
    currentIV[i] = iv[i];
  }

  for (size_t b = 0; b < blocks; ++b)
  {
    // get plaintext XOR IV
    for (size_t i = 0; i < blockSize; ++i)
    {
      workVector[i] = plaintext[b * blockSize + i] ^ currentIV[i];
    }

    aes256_encrypt_ecb(&ctx, workVector);

    // write to ciphertext, get new IV
    for (size_t i = 0; i < blockSize; ++i)
    {
      ciphertext[b * blockSize + i] = workVector[i];
      currentIV[i] = workVector[i];
    }
  }

  aes256_done(&ctx);
  return ciphertext;
}

std::vector<uint8_t> AES256CBCDecode(const std::vector<uint8_t>& cipher,
                                     const std::vector<uint8_t>& key,
                                     const std::vector<uint8_t>& iv)
{
  if (!(cipher.size() > 0) || (key.size() < 32) || (iv.size() < 32)) return std::vector<uint8_t>();

  aes256_context ctx;
  aes256_init(&ctx, key.data());

  const int blockSize = 16;
  size_t blocks = cipher.size() / blockSize;

  std::vector<uint8_t> plaintext(blockSize * blocks);

  uint8_t currentIV[blockSize];
  uint8_t nextIV[blockSize];
  uint8_t workVector[blockSize];

  for (int i = 0; i < blockSize; ++i)
  {
    currentIV[i] = iv[i];
  }

  for (int b = 0; b < blocks; ++b)
  {
    // get next cipher block and use ciphertext as next IV
    for (int i = 0; i < blockSize; ++i)
    {
      workVector[i] = cipher[b * blockSize + i];
      nextIV[i] = workVector[i];
    }

    aes256_decrypt_ecb(&ctx, workVector);

    // write to plaintext, XOR work vector with IV
    for (int i = 0; i < blockSize; ++i)
    {
      workVector[i] ^= currentIV[i];
      plaintext[b * blockSize + i] = workVector[i];
      currentIV[i] = nextIV[i];
    }
  }

  aes256_done(&ctx);

  // remove PKCS padding
  size_t paddedSize = plaintext.size();
  if (paddedSize % blockSize == 0)
  {
    int padBytes = plaintext[paddedSize - 1];
    if ((padBytes <= 16) && (padBytes < paddedSize))
    {
      plaintext.resize(paddedSize - padBytes);
    }
  }

  return plaintext;
}

bool collate(const TextFragment& a, const TextFragment& b)
{
  auto ia = a.begin();
  auto ib = b.begin();

  int iterEnds = 0;
  while (!iterEnds)
  {
    CodePoint ca = *ia;
    CodePoint cb = *ib;

    if (!validateCodePoint(ca)) return false;
    if (!validateCodePoint(cb)) return false;

    if (ca != cb)
    {
      // the code points differ.
      // compare codepoints, produce a result and bail.
      if (isLatin(ca) && isLatin(cb))
      {
        char la = tolower(ca);
        char lb = tolower(cb);

        if (la != lb)
        {
          // different letters
          return la < lb;
        }
        else
        {
          // different cases but same letter. define lower case as less within
          // letter.
          return ca > cb;
        }
      }
      else
      {
        // TODO collate other languages better using miniutf library.
        return ca < cb;
      }
    }
    else
    {
      ++ia;
      ++ib;
    }

    int aEnd = (ia == a.end());
    int bEnd = (ib == b.end());
    iterEnds = (aEnd << 1) | bEnd;
  }

  switch (iterEnds)
  {
    case 1:  // b ended but not a: a > b.
      return false;
    case 2:  // a ended but not b: a < b.
      return true;
    case 3:   // both ended, a == b.
    default:  // impossible
      return false;
  }
  return false;
}

#pragma mark Symbol utilities

Symbol addFinalNumber(Symbol sym, int n)
{
  TextFragment t(sym.getTextFragment(), textUtils::naturalNumberToText(n));
  return Symbol(t.getText());
}

Symbol stripFinalNumber(Symbol sym)
{
  const TextFragment& frag = sym.getTextFragment();
  size_t points = frag.lengthInCodePoints();

  // TODO make more readble using random access fragment class

  SmallStackBuffer<CodePoint, kShortFragmentSizeInCodePoints> temp(points + 1);
  CodePoint* buf = temp.data();

  // read into char32 array for random access
  int i = 0;
  for (CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return Symbol();
    buf[i++] = c;
  }

  // null terminate
  buf[points] = 0;

  // no final number? return
  if (!textUtils::isDigit(buf[points - 1])) return sym;

  // read backwards until non-digit
  size_t firstDigitPos = 0;
  for (size_t i = points - 2; i >= 0; --i)
  {
    char32_t c = buf[i];
    if (!textUtils::isDigit(c))
    {
      firstDigitPos = i + 1;
      break;
    }
  }

  ml::TextFragment subFrag(textUtils::subText(frag, 0, firstDigitPos));
  return subFrag.getText();
}

// if the symbol's text ends in a positive integer, return that number.
// Otherwise return 0.
int getFinalNumber(Symbol sym)
{
  // make temporary buffer of decoded code points, hopefully on stack
  const TextFragment& frag = sym.getTextFragment();
  size_t points = frag.lengthInCodePoints();

  // TODO make more readble using random access fragment class

  SmallStackBuffer<CodePoint, kShortFragmentSizeInCodePoints> decodedPoints(points + 1);
  CodePoint* buf = decodedPoints.data();

  // read into char32 array for random access
  int i = 0;
  for (CodePoint c : frag)
  {
    if (!validateCodePoint(c)) return 0;
    buf[i++] = c;
  }

  // null terminate char32_t string
  buf[i] = 0;

  // no final number? return
  if (!textUtils::isDigit(buf[i - 1])) return 0;

  // read backwards until non-digit
  int firstDigitPos = 0;
  for (i--; i >= 0; --i)
  {
    char32_t c = buf[i];
    if (!textUtils::isDigit(c))
    {
      firstDigitPos = i + 1;
      break;
    }
  }

  // note, null terminated char32_t string needed
  int r = digitsToNaturalNumber(buf + firstDigitPos);
  return r;
}

Symbol stripFinalCharacter(Symbol sym)
{
  TextFragment frag = sym.getTextFragment();
  size_t len = frag.lengthInCodePoints();
  return Symbol(subText(frag, 0, len - 1));
}

#pragma mark NameMaker

// base-26 arithmetic with letters (A = 0) produces A, B, ... Z, BA, BB ...
const TextFragment NameMaker::nextName()
{
  std::vector<int> digits;
  const int base = 26;
  const char baseChar = 'A';
  int a, m, d, rem;

  a = index++;

  if (!a)
  {
    digits.push_back(0);
  }
  else
    while (a)
    {
      d = a / base;
      m = d * base;
      rem = a - m;
      digits.push_back(rem);
      a = d;
    }

  int c = 0;
  while (digits.size() && (c < maxLen - 1))
  {
    d = digits.back();
    digits.pop_back();

    buf[c++] = static_cast<char>(d) + baseChar;
  }

  buf[c++] = 0;
  return TextFragment(buf);
}

class NoiseGen
{
 public:
  NoiseGen() : mSeed(0) {}
  ~NoiseGen() {}

  inline void step() { mSeed = mSeed * 0x0019660D + 0x3C6EF35F; }

  inline uint32_t getIntSample()
  {
    step();
    return mSeed;
  }

  void reset() { mSeed = 0; }

 private:
  uint32_t mSeed = 0;
};

static const char kLetters[33] = "aabcdeefghijklmnnoopqrssttuvwxyz";
std::vector<Symbol> vectorOfNonsenseSymbols(int len)
{
  NoiseGen randSource;
  std::vector<Symbol> words;
  for (int i = 0; i < len; ++i)
  {
    std::string newStr;
    uint32_t r32 = randSource.getIntSample() >> 16;
    int wordLen = (r32 & 7) + 3;

    for (int j = 0; j < wordLen; ++j)
    {
      r32 = randSource.getIntSample() >> 16;
      int idx = (r32 & 31);
      newStr += (kLetters[idx]);
    }
    words.push_back(Symbol(newStr.c_str()));
  }
  return words;
}

ml::Text formatNumber(const float number, const int digits, const int precision, const bool doSign,
                      Symbol mode) throw()
{
  const std::vector<ml::Text> pitchNames{"A",  "A#", "B", "C",  "C#", "D",
                                         "D#", "E",  "F", "F#", "G",  "G#"};

  const int bufLength = 16;
  char numBuf[bufLength] = {0};
  char format[bufLength] = {0};
  float tweakedNumber;

  // get digits to display
  int m = (precision > 0) ? std::max(digits, precision + 1) : digits;
  int d = ceil(log10f(fabs(number) + 1.));
  int p = (d + precision > m) ? m - d : precision;
  p = std::max(p, 0);

  //  printf("---------number: %-+10.2f\n", number);
  //  printf("---------number: %-+10.8f\n", number);
  //  printf("max: %d, digits: %d, after decimal: %d\n", m, d, p);

  tweakedNumber = number;
  if (mode == "default")
  {
    if (doSign)
    {
      snprintf(format, bufLength, "X-+0%1d.%1df", m, p);
    }
    else
    {
      snprintf(format, bufLength, "X-0%1d.%1df", m, p);
    }
    format[0] = 37;  // '%'
    snprintf(numBuf, bufLength, format, tweakedNumber);
  }
  else if (mode == "ratio")
  {
    bool done = false;
    for (int a = 1; a <= 8 && !done; ++a)
    {
      for (int b = 1; b <= 4 && !done; ++b)
      {
        if (fabs(number - (float)a / (float)b) < 0.001)
        {
          snprintf(numBuf, bufLength, "%d/%d", a, b);
          done = true;
        }
      }
    }
    if (!done)
    {
      if (doSign)
      {
        snprintf(format, bufLength, "X-+0%1d.%1df", m, p);
      }
      else
      {
        snprintf(format, bufLength, "X-0%1d.%1df", m, p);
      }
      format[0] = 37;  // '%'
      snprintf(numBuf, bufLength, format, tweakedNumber);
    }
  }
  else if (mode == "pitch1")  // just show As
  {
    int octave = log2(number / (27.5f - 0.01f));
    float quant = (pow(2.f, (float)octave) * 27.5f);
    float distFromOctave = fabs(number - quant);
    if (distFromOctave < 0.01)
    {
      snprintf(format, bufLength, "X-0%1d.%1df\nA%d", m, p, octave);
    }
    else
    {
      snprintf(format, bufLength, "X-0%1d.%1df", m, p);
    }
    format[0] = 37;  // '%'
    snprintf(numBuf, bufLength, format, tweakedNumber);
  }
  else if (mode == "pitch2")  // show all notes
  {
    int note = log2f(number / (27.5f - 0.01f)) * 12.f;
    float quantizedNotePitch = (pow(2.f, (float)note / 12.f) * 27.5f);
    float distFromNote = fabs(number - quantizedNotePitch);
    if (distFromNote < 0.01)
    {
      const int octaveFromC = (note - 3) / 12;
      snprintf(format, bufLength, "X-0%1d.%1df\n%s%d", m, p, pitchNames[note % 12].getText(),
               octaveFromC);
    }
    else
    {
      snprintf(format, bufLength, "X-0%1d.%1df", m, p);
    }
    format[0] = 37;  // '%'
    snprintf(numBuf, bufLength, format, tweakedNumber);
  }
  else if (mode == "db")
  {
    if (doSign)
    {
      snprintf(format, bufLength, "X-+0%1d.%1dfdB", m, p);
    }
    else
    {
      snprintf(format, bufLength, "X-0%1d.%1dfdB", m, p);
    }
    format[0] = 37;  // '%'
    snprintf(numBuf, bufLength, format, tweakedNumber);
  }

  return Text(numBuf);
}

}  // namespace textUtils
}  // namespace ml
