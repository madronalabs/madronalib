//
//  MLTextUtils.cpp
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#include "MLTextUtils.h"
#include "../DSP/MLDSPGens.h" // for RandomSource TODO replace

#pragma mark string utilities

namespace ml { namespace textUtils {
	
	using namespace utf;
	
	bool isDigit(char32_t c)
	{
		if (c >= '0' && c <= '9')
			return true;
		return false;
	}
	
	char * spaceStr( int numIndents )
	{
		static char * pBuf = (char *)"                                                   ";
		static int len = (int)strlen(pBuf);
		int n = numIndents*2;
		if (n > len) n = len;
		return &pBuf[len - n];
	}
	
	TextFragment positiveIntToDigits(int i)
	{
		const int kMLMaxNumberDigits = 14;
		
		char buf[kMLMaxNumberDigits + 2] = {0};
		char *p = buf + kMLMaxNumberDigits + 1;	
		char *end = p;
		
		// null-terminate the string
		*end = 0;
		
		// work backwards
		do 
		{
			*--p = '0' + (i % 10);
			i /= 10;
		} 
		while (i != 0);
		return TextFragment(p, end - p);
	}
	
	const char* naturalNumberToDigits(int value, char* pDest) 
	{
		const int base = 10;
		char* ptr = pDest, *ptr1 = pDest, tmp_char;
		int tmp_value;
		
		if(value <= 0)
		{
			*pDest = '0';
			*(++pDest) = '\0';
			return pDest;
		}
		do 
		{
			tmp_value = value;
			value /= base;
			*ptr++ = "0123456789abcdef"[tmp_value - value*base];
		} while ( value > 0 );
		
		*ptr-- = '\0';
		while(ptr1 < ptr) 
		{
			tmp_char = *ptr;
			*ptr-- = *ptr1;
			*ptr1++ = tmp_char;
		}
		return pDest;
	}
	
	int digitsToPositiveInt(const char32_t* p)
	{
		int v = 0;
		int l = 0;
		int d;
		char c;
		
		const int kMLMaxNumberDigits = 14;
		
		while (p[l] && (l < kMLMaxNumberDigits-1))
		{
			c = p[l];
			if (c >= '0' && c <= '9')
				d = (c - '0');
			else
				break;
			v = (v * 10) + d;
			l++;
		}
		return v;
	}
	
	const size_t countCodePoints(const TextFragment& frag)
	{
		utf::stringview<const char*> sv(frag.text, frag.text + frag.lengthInBytes);
		return sv.codepoints();
	}
	
	int findFirst(const TextFragment& frag, const codepoint_type c)
	{
		auto first = codepoint_iterator<const char*>(frag.text);
		auto last = codepoint_iterator<const char*>(frag.text + frag.lengthInBytes);
		int i=0;
		int r = -1;
		for (auto it = first; it != last; ++it) 
		{
			if(c == *it)
			{
				r = i;
				break;
			}
			i++;
		}	
		return r;
	}
	
	int findLast(const TextFragment& frag, const codepoint_type c)
	{
		auto first = codepoint_iterator<const char*>(frag.text);
		auto last = codepoint_iterator<const char*>(frag.text + frag.lengthInBytes);
		int i=0;
		int r = -1;
		for (auto it = first; it != last; ++it) 
		{
			if(c == *it)
			{
				r = i;
			}
			i++;
		}	
		return r;
	}
	
	TextFragment subText(const TextFragment& frag, int start, int end)
	{
		// this impl does an unneccesary copy, to keep TextFragment very simple for now.
		
		if(start >= end) return TextFragment("", 0);
		
		// temp buffer on stack big enough to hold whole input fragment if needed.
		// we won't know the output fragment size in bytes until iterating the code points. 
		char buf[frag.lengthInBytes];
		std::fill(buf, buf+frag.lengthInBytes, 0);
		char* pb = buf;
		
		auto first = codepoint_iterator<const char*>(frag.text);		
		auto it = first;
		for(int i=0; i<start; ++i)
		{
			++it;
		}
		
		for (int i=0; i<end - start; ++i) 
		{
			// write the codepoint as UTF-8 to the buffer
			pb = utf::internal::utf_traits<utf::utf8>::encode(*it, pb);
			++it;
		}	
		
		return TextFragment(buf, pb - buf);
	}
	
	TextFragment stripExtension(const TextFragment& frag)
	{
		int dotLoc = findLast(frag, '.');
		if(dotLoc >= 0)
		{
			return subText(frag, 0, dotLoc);
		}
		
		return frag;
	}
	
	TextFragment getShortName(const TextFragment& frag)
	{
		int slashLoc = findLast(frag, '/');
		if(slashLoc >= 0)
		{
			return subText(frag, slashLoc + 1, countCodePoints(frag));
		}
		
		return frag;
	}
	
	TextFragment getPath(const TextFragment& frag)
	{
		int slashLoc = findLast(frag, '/');
		if(slashLoc >= 0)
		{
			return subText(frag, 0, slashLoc);
		}
		
		return frag;
	}
	
	bool beginsWith(TextFragment fa, TextFragment fb)
	{
		int lenA = fa.lengthInBytes;
		int lenB = fb.lengthInBytes;
		
		if(lenB > lenA) return false;
		bool r = true;
		
		for(int i=0; i<lenB; ++i)
		{
			if(fa.text[i] != fb.text[i])
			{
				r = false;
				break;
			}
		}

		return r;
	}
	
	bool endsWith(TextFragment fa, TextFragment fb)
	{
		int lenA = fa.lengthInBytes;
		int lenB = fb.lengthInBytes;
		
		if(lenB > lenA) return false;
		bool r = true;
		
		for(int i=0; i<lenB; ++i)
		{
			if(fa.text[lenA - lenB + i] != fb.text[i])
			{
				r = false;
				break;
			}
		}
		
		return r;
	}

	
#pragma mark Symbol utilities
	
	
	Symbol addFinalNumber(Symbol sym, int n)
	{
		TextFragment t(sym.getTextFragment() + TextFragment(textUtils::positiveIntToDigits(n)));
		return Symbol(t.text);
	}
	
	Symbol stripFinalNumber(Symbol sym)
	{
		const TextFragment& frag = sym.getTextFragment();
		utf::stringview<const char*> sv(frag.text, frag.text + frag.lengthInBytes);
		int points = sv.codepoints();
		char32_t buf[points];
		
		// read into char32 array for random access
		auto first = utf::codepoint_iterator<const char*>(frag.text);
		auto last = utf::codepoint_iterator<const char*>(frag.text + frag.lengthInBytes);
		int i=0;
		for (auto it = first; it != last; ++it) 
		{
			char32_t c = *it;
			buf[i++] = c;
		}			
		
		// no final number? return
		if(!textUtils::isDigit(buf[points - 1])) return sym;
		
		// read backwards until non-digit
		int firstDigitPos = 0;
		for(int i=points - 2; i >= 0; --i)
		{
			char32_t c = buf[i];
			if(!textUtils::isDigit(c))
			{
				firstDigitPos = i + 1; break;
			}
		}
		
		return Symbol(textUtils::subText(frag, 0, firstDigitPos).text);
	}
	
	// if the symbol's text ends in a natural number, return that number. Otherwise return 0.
	int getFinalNumber(Symbol sym)
	{
		const TextFragment& frag = sym.getTextFragment();
		utf::stringview<const char*> sv(frag.text, frag.text + frag.lengthInBytes);
		int points = sv.codepoints();
		char32_t buf[points];
		
		// read into char32 array for random access
		auto first = utf::codepoint_iterator<const char*>(frag.text);
		auto last = utf::codepoint_iterator<const char*>(frag.text + frag.lengthInBytes);
		int i=0;
		for (auto it = first; it != last; ++it) 
		{
			char32_t c = *it;
			buf[i++] = c;
		}			
		
		// no final number? return
		if(!textUtils::isDigit(buf[points - 1])) return 0;
		
		// read backwards until non-digit
		int firstDigitPos = 0;
		for(int i=points - 2; i >= 0; --i)
		{
			char32_t c = buf[i];
			if(!textUtils::isDigit(c))
			{
				firstDigitPos = i + 1; break;
			}
		}
		
		return digitsToPositiveInt(buf + firstDigitPos);
	}
	
	std::vector<Symbol> parsePath(const char* pathStr)
	{
		std::vector<Symbol> path;
		int pathStrBytes = strlen(pathStr);		
		char UTF8buf[pathStrBytes];
		std::fill(UTF8buf, UTF8buf+pathStrBytes, 0);
		char* beginPoint = UTF8buf;
		char* beginSymbol = UTF8buf;
		char* endPoint = UTF8buf;
		
		auto first = utf::codepoint_iterator<const char*>(pathStr);		
		auto it = first;
		int pointSizeAsUTF8;
		char c = 0;
		do
		{
			do
			{
				// write the codepoint as UTF-8 to the buffer and advance pb
				endPoint = utf::internal::utf_traits<utf::utf8>::encode(*it, beginPoint);				
				pointSizeAsUTF8 = endPoint - beginPoint;
				
				// if we have a one-byte character, see if it's a slash
				c = (pointSizeAsUTF8 == 1) ? *it : -1;
				beginPoint = endPoint;
				++it;
			}	
			while((c != '/') && (c != 0));
			
			int newSymbolBytes = (endPoint - beginSymbol) - 1;
			path.emplace_back(Symbol(beginSymbol, newSymbolBytes));
			beginSymbol = endPoint;
		}
		while(c != 0);
		
		return path;
	}
	
#pragma mark NameMaker
	
	// base-26 arithmetic with letters (A = 0) produces A, B, ... Z, BA, BB ...
	const Symbol NameMaker::nextName()
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
		else while(a)
		{
			d = a / base;
			m = d * base;
			rem = a - m;
			digits.push_back(rem);
			a = d;
		}
		
		int c = 0;
		while(digits.size() && (c < maxLen - 1))
		{
			d = digits.back();
			digits.pop_back();
			
			buf[c++] = static_cast<char>(d) + baseChar;
		}
		
		buf[c++] = 0;
		return Symbol(buf);
	}
	
	static const char kLetters[33] = "aabcdeefghijklmnnoopqrssttuvwxyz";
	std::vector<Symbol> vectorOfNonsenseSymbols( int len )
	{
		ml::RandomSource randSource;
		std::vector<Symbol> words;
		for(int i = 0; i < len; ++i)
		{
			std::string newStr;
			uint32_t r32 = randSource.getIntSample() >> 16;
			int wordLen = (r32 & 7) + 3;
			
			for(int j=0; j<wordLen; ++j)
			{
				r32 = randSource.getIntSample() >> 16;
				int idx = (r32 & 31);
				newStr += (kLetters[idx]);
			}
			words.push_back(Symbol(newStr.c_str()));
		}
		return words;
	}
} } // ml:textUtils
