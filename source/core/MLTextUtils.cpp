//
//  MLTextUtils.cpp
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#include "MLTextUtils.h"

#pragma mark string utilities

namespace ml { namespace textUtils {
	
	using namespace utf;
	
	static const int npos = -1;
	
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
		return (TextFragment(p, end - p));
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
	
	int findFirst(const TextFragment& frag, const codepoint_type c)
	{
		int r = npos;
		if(!frag) return r;
		auto first = codepoint_iterator<const char*>(frag.getText());
		auto last = codepoint_iterator<const char*>(frag.getText() + frag.lengthInBytes());
		int i=0;
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
		int r = npos;
		if(!frag) return r;
		auto first = codepoint_iterator<const char*>(frag.getText());
		auto last = codepoint_iterator<const char*>(frag.getText() + frag.lengthInBytes());
		int i=0;
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
		if(!frag) return TextFragment();
		if(start >= end) return TextFragment();
		
		// temp buffer on stack big enough to hold whole input fragment if needed.
		// we won't know the output fragment size in bytes until iterating the code points. 
		char buf[frag.lengthInBytes()];
		std::fill(buf, buf+frag.lengthInBytes(), 0);
		char* pb = buf;
		
		auto first = codepoint_iterator<const char*>(frag.getText());		
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
	
	TextFragment stripFileExtension(const TextFragment& frag)
	{
		int dotLoc = findLast(frag, '.');
		if(dotLoc >= 0)
		{
			return subText(frag, 0, dotLoc);
		}		
		return frag;
	}
	
	TextFragment getShortFileName(const TextFragment& frag)
	{
		int slashLoc = findLast(frag, '/');
		if(slashLoc >= 0)
		{
			return subText(frag, slashLoc + 1, frag.lengthInCodePoints());
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
	
#pragma mark Symbol utilities
	
	Symbol addFinalNumber(Symbol sym, int n)
	{
		TextFragment t(sym.getTextFragment() + TextFragment(textUtils::positiveIntToDigits(n)));
		return Symbol(t.getText());
	}
	
	Symbol stripFinalNumber(Symbol sym)
	{
		const TextFragment& frag = sym.getTextFragment();
		utf::stringview<const char*> sv(frag.getText(), frag.getText() + frag.lengthInBytes());
		int points = sv.codepoints();
		char32_t buf[points];
		
		// read into char32 array for random access
		auto first = utf::codepoint_iterator<const char*>(frag.getText());
		auto last = utf::codepoint_iterator<const char*>(frag.getText() + frag.lengthInBytes());
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
		
		return Symbol(textUtils::subText(frag, 0, firstDigitPos).getText());
	}

	// if the symbol's text ends in a natural number, return that number. Otherwise return 0.
	int getFinalNumber(Symbol sym)
	{
		const TextFragment& frag = sym.getTextFragment();
		utf::stringview<const char*> sv(frag.getText(), frag.getText() + frag.lengthInBytes());
		int points = sv.codepoints();
		char32_t buf[points];
		
		// read into char32 array for random access
		auto first = utf::codepoint_iterator<const char*>(frag.getText());
		auto last = utf::codepoint_iterator<const char*>(frag.getText() + frag.lengthInBytes());
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

	Symbol stripFinalCharacter(Symbol sym)
	{
		TextFragment frag = sym.getTextFragment();
		int len = frag.lengthInCodePoints();
		return Symbol(subText(frag, 0, len - 1));
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
